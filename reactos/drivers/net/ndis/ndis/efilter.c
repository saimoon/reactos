/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/efilter.c
 * PURPOSE:     Ethernet filter functions
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Vizzini (vizzini@plasmic.com)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "ndissys.h"

BOOLEAN
EXPORT
EthCreateFilter(
    IN  UINT                MaximumMulticastAddresses,
    IN  PUCHAR              AdapterAddress,
    OUT PETH_FILTER         * Filter)
/*
 * FUNCTION: Construct an ethernet filter
 * ARGUMENTS:
 *     MaximumMulticastAddresses: Maximum number of multicast adderesses.
 *     AdapterAddress: Current ethernet address of the adapter.
 *     Filter: The created filter on successful return.
 * RETURNS:
 *     TRUE if the filter was created
 *     FALSE otherwise
 * NOTE:
 *     - This function is no longer exported and intentionally doesn't
 *       follow the W2K prototype. It was deprecated since NDIS 4 so it
 *       shouldn't be problem.
 */
{
  PETH_FILTER NewFilter;

  NewFilter = ExAllocatePool(NonPagedPool, sizeof(ETH_FILTER));
  if (NewFilter != NULL) 
    {
      RtlZeroMemory(NewFilter, sizeof(ETH_FILTER));
      NewFilter->MaxMulticastAddresses = MaximumMulticastAddresses;
      RtlCopyMemory(NewFilter->AdapterAddress, AdapterAddress, ETH_LENGTH_OF_ADDRESS);
      *Filter = NewFilter;
      return TRUE;
    }
  return FALSE;
}


VOID
EXPORT
EthDeleteFilter(
    IN  PETH_FILTER Filter)
{
    ExFreePool(Filter);
}


/*
 * @unimplemented
 */
VOID
EXPORT
EthFilterDprIndicateReceive(
    IN	PETH_FILTER Filter,
    IN	NDIS_HANDLE MacReceiveContext,
    IN	PCHAR       Address,
    IN	PVOID       HeaderBuffer,
    IN	UINT        HeaderBufferSize,
    IN	PVOID       LookaheadBuffer,
    IN	UINT        LookaheadBufferSize,
    IN	UINT        PacketSize)
/*
 * FUNCTION: Receive indication function for Ethernet devices
 * ARGUMENTS:
 *     MiniportAdapter     = Miniport Adapter Handle (PLOGICAL_ADAPTER)
 *     MacReceiveContext   = MAC receive context handle
 *     Address             = Pointer to destination Ethernet address
 *     HeaderBuffer        = Pointer to Ethernet header buffer
 *     HeaderBufferSize    = Size of Ethernet header buffer
 *     LookaheadBuffer     = Pointer to lookahead buffer
 *     LookaheadBufferSize = Size of lookahead buffer
 *     PacketSize          = Total size of received packet
 */
{
    MiniIndicateData((PLOGICAL_ADAPTER)Filter->Miniport,
		     MacReceiveContext,
		     HeaderBuffer,
		     HeaderBufferSize,
		     LookaheadBuffer,
		     LookaheadBufferSize,
		     PacketSize);
}


/*
 * @unimplemented
 */
VOID
EXPORT
EthFilterDprIndicateReceiveComplete(
    IN  PETH_FILTER Filter)
/*
 * FUNCTION: Receive indication complete function for Ethernet devices
 * ARGUMENTS:
 *     Filter = Pointer to Ethernet filter
 */
{
  KIRQL OldIrql;
  PLIST_ENTRY CurrentEntry;
  PLOGICAL_ADAPTER Adapter;
  PADAPTER_BINDING AdapterBinding;

  NDIS_DbgPrint(DEBUG_MINIPORT, ("Called.\n"));

  if( !Filter ) return;

  Adapter = (PLOGICAL_ADAPTER)Filter->Miniport;

  NDIS_DbgPrint(MAX_TRACE, ("acquiring miniport block lock\n"));
  KeAcquireSpinLock(&Adapter->NdisMiniportBlock.Lock, &OldIrql);
    {
      CurrentEntry = Adapter->ProtocolListHead.Flink;

      while (CurrentEntry != &Adapter->ProtocolListHead) 
        {
          AdapterBinding = CONTAINING_RECORD(CurrentEntry, ADAPTER_BINDING, AdapterListEntry);

          (*AdapterBinding->ProtocolBinding->Chars.ReceiveCompleteHandler)(
              AdapterBinding->NdisOpenBlock.NdisCommonOpenBlock.ProtocolBindingContext);

          CurrentEntry = CurrentEntry->Flink;
        }
    }
  KeReleaseSpinLock(&Adapter->NdisMiniportBlock.Lock, OldIrql);
}

/* EOF */
