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
#ifndef _include_sourcemod_core_logic_DependencyGraph_h_
#define _include_sourcemod_core_logic_DependencyGraph_h_

#include <amtl/am-deque.h>
#include <amtl/am-hashset.h>
#include <amtl/am-vector.h>
#include <amtl/am-function.h>

class CPlugin;

namespace SourceMod {

class PluginGraph
{
public:
	PluginGraph(CPlugin *root);
	~PluginGraph();

protected:
	// Compute the dependency graph. If any new plugins were discovered since
	// the last time the dependency graph was computed, returns true.
	bool ComputeDependencies();

private:
	void AddToFullList(CPlugin *plugin);
	void AddToWorklist(CPlugin *plugin);

private:
	struct PluginPtrHashPolicy {
		static uint32_t hash(CPlugin *value) {
			return ke::HashPointer(value);
		}
		static bool matches(CPlugin *a, CPlugin *b) {
			return a == b;
		}
	};
	typedef ke::HashSet<CPlugin *, PluginPtrHashPolicy> PluginSet;

protected:
	CPlugin *root_;

	// The final list of plugins in rough RPO order (with cycles broken), and
	// the membership test of whether or not a plugin is in the final list.
	ke::Vector<CPlugin *> full_list_;
	PluginSet in_full_list_;

	// The difference of unload_list_ after each call to ComputeDependencies.
	ke::Vector<CPlugin *> change_list_;

private:
	// The work queue, and membership test of whether or not a plugin was ever
	// in the work queue.
	ke::Deque<CPlugin *> work_queue_;
	PluginSet in_worklist_;
};

class PluginEvictionGraph : public PluginGraph
{
public:
	PluginEvictionGraph(CPlugin *root);
	~PluginEvictionGraph();

	void DoEviction();
	void ForEach(const ke::Lambda<void(CPlugin*)> &callback);

private:
	void FireDependencyCallbacks(CPlugin *plugin);
	void Unlink(CPlugin *plugin);
};

} // namespace SourceMod

#endif // _include_sourcemod_core_logic_DependencyGraph_h_
