// vim: set ts=4 sw=4 tw=99 noet :
// =============================================================================
// SourceMod
// Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
// =============================================================================
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License, version 3.0, as published by the
// Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.
//
// As a special exception, AlliedModders LLC gives you permission to link the
// code of this program (as well as its derivative works) to "Half-Life 2," the
// "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
// by the Valve Corporation.  You must obey the GNU General Public License in
// all respects for all other code used.  Additionally, AlliedModders LLC grants
// this exception to all derivative works.  AlliedModders LLC defines further
// exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
// or <http://www.sourcemod.net/license.php>.
#include "PluginGraph.h"
#include "PluginSys.h"

PluginGraph::PluginGraph(CPlugin *root)
	: root_(root)
{
}

PluginGraph::~PluginGraph()
{
}

bool
PluginGraph::ComputeDependencies()
{
	// Reset the work queue state.
	in_worklist_.clear();
	assert(work_queue_.empty());

	// Reset the differential output list.
	change_list_.clear();

	// The work queue is a deque, we append work to the back and remove it
	// from the front. This means the output list is roughly in postorder,
	// where the first item is the root and the last item is the right-most
	// leaf. Plugin dependencies can form cycles, but we break these using
	// the in_worklist_ test.
	while (!work_queue_.empty()) {
		CPlugin *parent = work_queue_.popFrontCopy();
		AddToFullList(parent);

		parent->ForEachDependent([this, parent] (CPlugin *child) -> void {
			if (in_worklist_.has(child))
				return;
			if (!child->IsStronglyDependentOn(parent))
				return;
			AddToWorklist(child);
		});
	}

	return change_list_.length() != 0;
}

void
PluginGraph::AddToWorklist(CPlugin *plugin)
{
	assert(!in_worklist_.has(plugin));
	in_worklist_.add(plugin);
	work_queue_.append(plugin);
}

void
PluginGraph::AddToFullList(CPlugin *plugin)
{
	if (in_full_list_.has(plugin))
		return;
	full_list_.append(plugin);
	change_list_.append(plugin);
	in_full_list_.add(plugin);
}

PluginEvictionGraph::PluginEvictionGraph(CPlugin *root)
	: PluginGraph(root)
{
}

PluginEvictionGraph::~PluginEvictionGraph()
{
}

void
PluginEvictionGraph::DoEviction()
{
	// Compute the dependency graph, then evict plugins. We iterate to a
	// fixpoint since firing dependency callbacks could (in extremely rare
	// cases) cause the dependency graph to change - for example via more
	// eviction callbacks firing or by new plugins loading.
	while (ComputeDependencies()) {
		for (size_t i = change_list_.length() - 1; i < change_list_.length(); i--)
			FireDependencyCallbacks(change_list_[i]);
	}

	for (size_t i = full_list_.length() - 1; i < full_list_.length(); i--)
		Unlink(full_list_[i]);
}

void
PluginEvictionGraph::FireDependencyCallbacks(CPlugin *plugin)
{
	plugin->LibraryActions(LibraryAction_Removed);
	plugin->SetPaused();
}

void
PluginEvictionGraph::Unlink(CPlugin *parent)
{
	parent->ForEachDependent([this, parent] (CPlugin *child) -> void {
		// Note: we skip this error for the root plugin since the caller is
		// responsible for its messaging.
		//
		// We allow the pause state to pass this test, since we paused plugins
		// earlier in the FireDependencyCallbacks pass.
		if (child != root_ &&
			(child->GetStatus() == Plugin_Running ||
			 child->GetStatus() == Plugin_Loaded ||
			 child->GetStatus() == Plugin_Paused) &&
		    child->IsStronglyDependentOn(parent))
		{
			child->DependencyError(Plugin_Error, "Depends on plugin: %s", parent->GetFilename());
		}

		// For safety, zap native bindings for fake natives. This is probably
		// not needed since ShareSys should be doing it.
		child->UnlinkNativesOwnedBy(parent);
	});

	// Zap incoming references to this plugin.
	g_PluginSys.ForEachPlugin([parent] (CPlugin *child) -> void {
		child->ToNativeOwner()->DropRefsTo(parent);
	});

	// Zap links in the native ownership system.
	parent->ToNativeOwner()->DropEverything();

	if (parent != root_)
		g_PluginSys.Evict(parent);
}
