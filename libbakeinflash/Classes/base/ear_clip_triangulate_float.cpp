// ear_clip_triangulate_float.cpp	-- Thatcher Ulrich 2006

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Triangulation, specialized for floating-point coords.

#include "bakeinflash/bakeinflash.h"
#include "base/ear_clip_triangulate.h"
#include "base/ear_clip_triangulate_impl.h"


namespace ear_clip_triangulate {
	void compute(
		array<float>* results,
		int path_count,
		const array<float> paths[],
		int debug_halt_step,
		array<float>* debug_edges)
	{
		ear_clip_triangulate::compute_triangulation<float>(results, path_count, paths, debug_halt_step, debug_edges);
	}

	void compute(
		array<float>* results,
		const array<bakeinflash::path_part>&	path_parts,
		int index,
		int debug_halt_step,
		array<float>* debug_edges)
	{
		ear_clip_triangulate::compute_triangulation<float>(results, path_parts, index, debug_halt_step, debug_edges);
	}

}

