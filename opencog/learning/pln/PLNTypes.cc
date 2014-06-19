/*
 * opencog/learning/pln/PLNTypes.cc
 *
 * Copyright (C) 2014 Cosmo Harrigan
 * All Rights Reserved
 *
 * Written by Cosmo Harrigan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <opencog/server/Module.h>
#include "opencog/learning/pln/atom_types.definitions"

#define INHERITANCE_FILE "opencog/learning/pln/atom_types.inheritance"
#define INITNAME pln_types_init

#include <opencog/atomspace/atom_types.cc>

using namespace opencog;
TRIVIAL_MODULE(PLNTypesModule)
DECLARE_MODULE(PLNTypesModule)
