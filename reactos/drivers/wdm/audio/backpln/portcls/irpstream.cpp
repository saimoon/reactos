/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/irpstream.cpp
 * PURPOSE:         IRP Stream handling
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"


class CIrpQueue : public IIrpQueue
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    STDMETHODIMP_(ULONG) AddRef()
    {
        InterlockedIncrement(&m_Ref);
        return m_Ref;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        InterlockedDecrement(&m_Ref);

        if (!m_Ref)
        {
            delete this;
            return 0;
        }
        return m_Ref;
    }
    IMP_IIrpQueue;
    CIrpQueue(IUnknown *OuterUnknown){}
    virtual ~CIrpQueue(){}

protected:
    ULONG m_CurrentOffset;
    LONG m_NumMappings;
    ULONG m_NumDataAvailable;
    BOOL m_StartStream;
    KSPIN_CONNECT * m_ConnectDetails;
    PKSDATAFORMAT_WAVEFORMATEX m_DataFormat;

    KSPIN_LOCK m_IrpListLock;
    LIST_ENTRY m_IrpList;
    LIST_ENTRY m_FreeIrpList;
    PIRP m_Irp;
    PVOID m_SilenceBuffer;

    ULONG m_OutOfMapping;
    ULONG m_MaxFrameSize;
    ULONG m_Alignment;
    ULONG m_MinimumDataThreshold;

    LONG m_Ref;

};

NTSTATUS
NTAPI
CIrpQueue::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    if (IsEqualGUIDAligned(refiid, IID_IUnknown))
    {
        *Output = PVOID(PUNKNOWN(this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CIrpQueue::Init(
    IN KSPIN_CONNECT *ConnectDetails,
    IN PKSDATAFORMAT DataFormat,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG FrameSize,
    IN ULONG Alignment,
    IN PVOID SilenceBuffer)
{
    m_ConnectDetails = ConnectDetails;
    m_DataFormat = (PKSDATAFORMAT_WAVEFORMATEX)DataFormat;
    m_MaxFrameSize = FrameSize;
    m_SilenceBuffer = SilenceBuffer;
    m_Alignment = Alignment;
    m_MinimumDataThreshold = ((PKSDATAFORMAT_WAVEFORMATEX)DataFormat)->WaveFormatEx.nAvgBytesPerSec / 3;

    InitializeListHead(&m_IrpList);
    InitializeListHead(&m_FreeIrpList);
    KeInitializeSpinLock(&m_IrpListLock);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CIrpQueue::AddMapping(
    IN PUCHAR Buffer,
    IN ULONG BufferSize,
    IN PIRP Irp)
{
    PKSSTREAM_HEADER Header;
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IoStack;

    PC_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    // get current irp stack location
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    PC_ASSERT(!Buffer);

    if (!Irp->MdlAddress)
    {
        // ioctl from KsStudio
        // Wdmaud already probes buffers, therefore no need to probe it again
        // probe the stream irp
        if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_WRITE_STREAM)
            Status = KsProbeStreamIrp(Irp, KSSTREAM_WRITE | KSPROBE_ALLOCATEMDL | KSPROBE_PROBEANDLOCK | KSPROBE_ALLOWFORMATCHANGE | KSPROBE_SYSTEMADDRESS, 0);
        else
            Status = KsProbeStreamIrp(Irp, KSSTREAM_READ| KSPROBE_ALLOCATEMDL | KSPROBE_PROBEANDLOCK | KSPROBE_ALLOWFORMATCHANGE | KSPROBE_SYSTEMADDRESS, 0);

        // check for success
        if (!NT_SUCCESS(Status))
        {
            DPRINT("KsProbeStreamIrp failed with %x\n", Status);
            return Status;
        }
    }
    // get the stream header
    Header = (PKSSTREAM_HEADER)Irp->AssociatedIrp.SystemBuffer;
    PC_ASSERT(Header);
    PC_ASSERT(Irp->MdlAddress);

    DPRINT("Size %u DataUsed %u FrameExtent %u SizeHeader %u NumDataAvailable %u OutputLength %u\n", Header->Size, Header->DataUsed, Header->FrameExtent, sizeof(KSSTREAM_HEADER), m_NumDataAvailable, IoStack->Parameters.DeviceIoControl.OutputBufferLength);

    Header->Data = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
    PC_ASSERT(Header->Data);
    //PC_ASSERT(Header->Size == IoStack->Parameters.DeviceIoControl.OutputBufferLength);

    // HACK
    Irp->Tail.Overlay.DriverContext[2] = (PVOID)Header;

    // sanity check
    PC_ASSERT(Header);

    // dont exceed max frame size
    //PC_ASSERT(m_MaxFrameSize >= Header->DataUsed);

    // increment num mappings
    InterlockedIncrement(&m_NumMappings);

    // increment num data available
    m_NumDataAvailable += Header->DataUsed;

    // mark irp as pending
    IoMarkIrpPending(Irp);

    // add irp to cancelable queue
    KsAddIrpToCancelableQueue(&m_IrpList, &m_IrpListLock, Irp, KsListEntryTail, NULL);

    // done
    return Status;
}

NTSTATUS
NTAPI
CIrpQueue::GetMapping(
    OUT PUCHAR * Buffer,
    OUT PULONG BufferSize)
{
    PIRP Irp;
    ULONG Offset;
    //PIO_STACK_LOCATION IoStack;
    PKSSTREAM_HEADER StreamHeader;

    // check if there is an irp in the partially processed
    if (m_Irp)
    {
        // use last irp
        if (m_Irp->Cancel == FALSE)
        {
            Irp = m_Irp;
            Offset = m_CurrentOffset;
        }
        else
        {
            // irp has been cancelled
            m_Irp->IoStatus.Status = STATUS_CANCELLED;
            IoCompleteRequest(m_Irp, IO_NO_INCREMENT);
            m_Irp = Irp = NULL;
        }
    }
    else
    {
        // get a fresh new irp from the queue
        m_Irp = Irp = KsRemoveIrpFromCancelableQueue(&m_IrpList, &m_IrpListLock, KsListEntryHead, KsAcquireAndRemoveOnlySingleItem);
        m_CurrentOffset = Offset = 0;
    }

    if (!Irp)
    {
        DPRINT("NoIrp\n");
        // no irp available, use silence buffer
        *Buffer = (PUCHAR)m_SilenceBuffer;
        *BufferSize = m_MaxFrameSize;
        // flag for port wave pci driver
        m_OutOfMapping = TRUE;
        // indicate flag to restart fast buffering
        m_StartStream = FALSE;
        return STATUS_SUCCESS;
    }

#if 0
    // get current irp stack location
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    // get stream header
    StreamHeader = (PKSSTREAM_HEADER)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;
#else
    // HACK get stream header
    StreamHeader = (PKSSTREAM_HEADER)Irp->Tail.Overlay.DriverContext[2];
#endif

    // sanity check
    PC_ASSERT(StreamHeader);

    // store buffersize
    if (StreamHeader->DataUsed)
        *BufferSize = StreamHeader->DataUsed - Offset;
    else
        *BufferSize = StreamHeader->FrameExtent - Offset;

    PC_ASSERT(*BufferSize);

    // store buffer
    *Buffer = &((PUCHAR)StreamHeader->Data)[Offset];

    // unset flag that no irps are available
    m_OutOfMapping = FALSE;

    return STATUS_SUCCESS;
}

VOID
NTAPI
CIrpQueue::UpdateMapping(
    IN ULONG BytesWritten)
{
    PKSSTREAM_HEADER StreamHeader;
    ULONG Size;

    if (!m_Irp)
    {
        // silence buffer was used
        return;
    }

    // get stream header
    StreamHeader = (PKSSTREAM_HEADER)m_Irp->Tail.Overlay.DriverContext[2];

    // sanity check
   // ASSERT(StreamHeader);

    // add to current offset
    m_CurrentOffset += BytesWritten;

    // decrement available data counter
    m_NumDataAvailable -= BytesWritten;

    if (StreamHeader->DataUsed)
        Size = StreamHeader->DataUsed;
    else
        Size = StreamHeader->FrameExtent;

    PC_ASSERT(Size);

    if (m_CurrentOffset >= Size)
    {
        // irp has been processed completly
        m_Irp->IoStatus.Status = STATUS_SUCCESS;

        // frame extend contains the original request size, DataUsed contains the real buffer size
         //is different when kmixer performs channel conversion, upsampling etc
        
        m_Irp->IoStatus.Information = Size;

        PC_ASSERT_IRQL(DISPATCH_LEVEL);
        MmUnlockPages(m_Irp->MdlAddress);
        IoFreeMdl(m_Irp->MdlAddress);
        m_Irp->MdlAddress = NULL;
        ExFreePool(m_Irp->AssociatedIrp.SystemBuffer);
        m_Irp->AssociatedIrp.SystemBuffer = NULL;

        // complete the request
        IoCompleteRequest(m_Irp, IO_SOUND_INCREMENT);
        // remove irp as it is complete
        m_Irp = NULL;
        m_CurrentOffset = 0;
    }
}

ULONG
NTAPI
CIrpQueue::NumMappings()
{

    // returns the amount of mappings available
    return m_NumMappings;
}

ULONG
NTAPI
CIrpQueue::NumData()
{
    // returns the amount of audio stream data available
    return m_NumDataAvailable;
}


BOOL
NTAPI
CIrpQueue::MinimumDataAvailable()
{
    BOOL Result;

    if (m_StartStream)
        return TRUE;

    if (m_MinimumDataThreshold < m_NumDataAvailable)
    {
        m_StartStream = TRUE;
        Result = TRUE;
    }
    else
    {
        Result = FALSE;
    }
    return Result;
}

BOOL
NTAPI
CIrpQueue::CancelBuffers()
{

    m_StartStream = FALSE;
    return TRUE;
}

VOID
NTAPI
CIrpQueue::UpdateFormat(
    PKSDATAFORMAT DataFormat)
{
    m_DataFormat = (PKSDATAFORMAT_WAVEFORMATEX)DataFormat;
    m_MinimumDataThreshold = m_DataFormat->WaveFormatEx.nAvgBytesPerSec / 3;
    m_StartStream = FALSE;
    m_NumDataAvailable = 0;
}

NTSTATUS
NTAPI
CIrpQueue::GetMappingWithTag(
    IN PVOID Tag,
    OUT PPHYSICAL_ADDRESS  PhysicalAddress,
    OUT PVOID  *VirtualAddress,
    OUT PULONG  ByteCount,
    OUT PULONG  Flags)
{
    PKSSTREAM_HEADER StreamHeader;
    PIRP Irp;

    *Flags = 0;
    PC_ASSERT(Tag != NULL);

    // get an irp from the queue
    Irp = KsRemoveIrpFromCancelableQueue(&m_IrpList, &m_IrpListLock, KsListEntryHead, KsAcquireAndRemoveOnlySingleItem);

    // check if there is an irp
    if (!Irp)
    {
        // no irp available
        m_OutOfMapping = TRUE;
        m_StartStream = FALSE;
        return STATUS_UNSUCCESSFUL;
    }

    // HACK get stream header
    StreamHeader = (PKSSTREAM_HEADER)Irp->Tail.Overlay.DriverContext[2];

    // store mapping in the free list
    ExInterlockedInsertTailList(&m_FreeIrpList, &Irp->Tail.Overlay.ListEntry, &m_IrpListLock);

    // return mapping
    *PhysicalAddress = MmGetPhysicalAddress(StreamHeader->Data);
    *VirtualAddress = StreamHeader->Data;
    *ByteCount = StreamHeader->DataUsed;

    // decrement mapping count
    InterlockedDecrement(&m_NumMappings);
    // decrement num data available
    m_NumDataAvailable -= StreamHeader->DataUsed;

    // store tag in irp
    Irp->Tail.Overlay.DriverContext[3] = Tag;

    // done
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CIrpQueue::ReleaseMappingWithTag(
    IN PVOID Tag)
{
    PIRP Irp;
    PLIST_ENTRY CurEntry;
    PKSSTREAM_HEADER StreamHeader;

    DPRINT("CIrpQueue::ReleaseMappingWithTag Tag %p\n", Tag);

    // remove irp from used list
    CurEntry = ExInterlockedRemoveHeadList(&m_FreeIrpList, &m_IrpListLock);
    // sanity check
    PC_ASSERT(CurEntry);

    // get irp from list entry
    Irp = (PIRP)CONTAINING_RECORD(CurEntry, IRP, Tail.Overlay.ListEntry);

    // HACK get stream header
    StreamHeader = (PKSSTREAM_HEADER)Irp->Tail.Overlay.DriverContext[2];

    // driver must release items in the same order
    PC_ASSERT(Irp->Tail.Overlay.DriverContext[3] == Tag);

    // irp has been processed completly
    Irp->IoStatus.Status = STATUS_SUCCESS;

    // frame extend contains the original request size, DataUsed contains the real buffer size
    // is different when kmixer performs channel conversion, upsampling etc
    
    Irp->IoStatus.Information = StreamHeader->FrameExtent;

    // free stream header, no tag as wdmaud.drv allocates it atm
    ExFreePool(StreamHeader);

    // complete the request
    IoCompleteRequest(Irp, IO_SOUND_INCREMENT);

    return STATUS_SUCCESS;
}

BOOL
NTAPI
CIrpQueue::HasLastMappingFailed()
{
    return m_OutOfMapping;
}

VOID
NTAPI
CIrpQueue::PrintQueueStatus()
{

}

VOID
NTAPI
CIrpQueue::SetMinimumDataThreshold(
    ULONG MinimumDataThreshold)
{

    m_MinimumDataThreshold = MinimumDataThreshold;
}

ULONG
NTAPI
CIrpQueue::GetMinimumDataThreshold()
{
    return m_MinimumDataThreshold;
}


NTSTATUS
NTAPI
NewIrpQueue(
    IN IIrpQueue **Queue)
{
    CIrpQueue *This = new(NonPagedPool, TAG_PORTCLASS)CIrpQueue(NULL);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->AddRef();

    *Queue = (IIrpQueue*)This;
    return STATUS_SUCCESS;
}

