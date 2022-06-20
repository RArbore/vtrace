/*
 * This file is part of vtrace-rs.
 * vtrace-rs is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 * vtrace-rs is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with vtrace-rs. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma kernel main

RWStructuredBuffer<uint> result : register(u0);

[numthreads(64,1,1)]
void main(uint thread_id : SV_GroupIndex, uint3 group_id : SV_GroupID) {
    result[thread_id + 64 * group_id.x] = group_id.x | thread_id;
}
