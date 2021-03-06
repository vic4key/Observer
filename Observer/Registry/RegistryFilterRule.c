#include "Includes.h"
#include "../Rule.h"
#include "../Log/Log.h"
#include "../Util/ResourceList.h"

OBSERVER_RESOURCE_LIST RegistryFilterRuleList;

_Use_decl_annotations_
NTSTATUS RegistryFilterAddRule(
	POBSERVER_REGISTRY_RULE Rule,
	POBSERVER_RULE_HANDLE RuleHandle
)
{
	static LONG64 RuleCounter = 0;
	PREGISTRY_FILTER_RULE_ENTRY RuleEntry = NULL;

	ULONG Length =	FIELD_OFFSET(REGISTRY_FILTER_RULE_ENTRY, Rule.Path) +
		((Rule->PathLength + 1) * sizeof(WCHAR));

	RuleEntry = REGISTRY_FILTER_ALLOCATE(
		Length,
		NonPagedPool
	);

	if (RuleEntry == NULL)
	{
		DEBUG_LOG("RegistryFilterAddRule: Out of memory");
		return STATUS_NO_MEMORY;
	}
	RtlCopyMemory(
		&RuleEntry->Rule,
		Rule,
		sizeof(OBSERVER_REGISTRY_RULE)
	);

	RtlCopyMemory(
		&RuleEntry->Rule.Path[0],
		&Rule->Path[0],
		RuleEntry->Rule.PathLength * sizeof(WCHAR)
	);

	RuleEntry->Rule.Path[RuleEntry->Rule.PathLength] = L'\0';
	RtlInitUnicodeString(&RuleEntry->Path, &RuleEntry->Rule.Path[0]);

	RuleEntry->Rule.ValueName[REGISTRY_RULE_VALUE_NAME_BUFFER_LENTGTH - 1] = L'\0';


	RuleHandle->RuleHandle = RuleEntry->RuleHandle.RuleHandle = InterlockedIncrement64(&RuleCounter);
	RuleHandle->RuleType = RuleEntry->RuleHandle.RuleType = RULE_TYPE_REGISTRY;

	RuleEntry->Refcount = 1;

	InsertResourceListHead(
		&RegistryFilterRuleList,
		&RuleEntry->ListEntry
	);

	return STATUS_SUCCESS;
}


_Use_decl_annotations_
NTSTATUS RegistryFilterRemoveRule(
	POBSERVER_RULE_HANDLE RuleHandle
)
{
	PLIST_ENTRY pEntry;

	if (RuleHandle->RuleType != RULE_TYPE_REGISTRY)
	{
		return STATUS_NOT_FOUND;
	}

	WLockResourceList(&RegistryFilterRuleList);

	for (
		pEntry = RegistryFilterRuleList.ListEntry.Flink;
		pEntry != &RegistryFilterRuleList.ListEntry;
		pEntry = pEntry->Flink
		)
	{
		PREGISTRY_FILTER_RULE_ENTRY CurrentEntry;
		CurrentEntry = CONTAINING_RECORD(pEntry, REGISTRY_FILTER_RULE_ENTRY, ListEntry);

		if (CurrentEntry->RuleHandle.RuleHandle == RuleHandle->RuleHandle)
		{
			RemoveEntryList(pEntry);
			if (InterlockedDecrement(&CurrentEntry->Refcount) == 0)
			{
				REGISTRY_FILTER_FREE(CurrentEntry);
			}
			WUnlockResourceList(&RegistryFilterRuleList);
			return STATUS_SUCCESS;
		}
	}
	WUnlockResourceList(&RegistryFilterRuleList);
	return STATUS_NOT_FOUND;
}

