/**********************************************************************
virtual_mol.h - Collection of OBAtom*, plus adapters for OBMol, etc.
***********************************************************************/

#ifndef VIRTUAL_MOL_H
#define VIRTUAL_MOL_H

#include <string>
#include <vector>
#include <set>
#include <utility>  // std::pair

#include <openbabel/babelconfig.h>
#include <openbabel/mol.h>
#include <openbabel/atom.h>

namespace OpenBabel
{

// Connections from interior of a fragment to external
typedef std::set< std::pair<OBAtom*, OBAtom*> > ConnIntToExt;
typedef std::set<OBAtom*> AtomSet;  // TODO consider using this alias throughout

class VirtualMol;

class VirtualMol {
// A collection of OBAtom*'s in a parent molecule.
// The alternative of copying OBMol's is more complicated, because that generates new OBAtom
// instances and it's more difficult to compare two OBAtom's if they intrinsically have different raw pointers.
// (Formerly, the code instead relied on searching for identical atomic number, position, etc.)
private:
	std::set<OBAtom*> _atoms;
	OBMol *_parent_mol;
public:
	// VirtualMol() = delete;  // does not make sense when there's a default parameter below.
	// Note: the map STL container requires a no-argument constructor in case map[key] has an unknown key.
	// For more info, see https://stackoverflow.com/questions/695645/why-does-the-c-map-type-argument-require-an-empty-constructor-when-using
	VirtualMol(OBMol *parent = NULL);
	VirtualMol(OBAtom *single_atom);
	OBMol* GetParent();
	int NumAtoms();
	std::set<OBAtom*> GetAtoms();
	bool HasAtom(OBAtom *a);
	bool AddAtom(OBAtom *a);
	bool RemoveAtom(OBAtom *a);
	bool AddVirtualMol(VirtualMol addition);  // consider writing as operator+= or +
	// Imports an OBMol fragment with copies of atoms in the same positions as _parent_mol
	int ImportCopiedFragment(OBMol *fragment);
	ConnIntToExt GetExternalBondsOrConns();  // map of external connections in the parent molecule
	OBMol ToOBMol(bool export_bonds = true, bool copy_bonds = true);
	void ToCIF(const std::string &filename, bool write_bonds = true);
	// TODO: consider implementing SMILES in a parent class due to OBConv
	// std::string ToSmiles();}
	std::vector<VirtualMol> Separate();
};


// Set up an iterator in the style of openbabel's obiter.h
// Unlike OBMol's, these atoms can be added/deleted, because they're
// virtual members of a set, not an actual OBMol iterator.
// Warning: these loops probably cannot be nested!
#define FOR_RW_ATOMS_OF_VMOL(a,v) \
	AtomSet __vset = v.GetAtoms(); \
	for (AtomSet::iterator a = __vset.begin(); a != __vset.end(); ++a)


} // end namespace OpenBabel
#endif // VIRTUAL_MOL_H

//! \file virtual_mol.h
//! \brief virtual_mol.h - Collection of OBAtom*, plus adapters for OBMol, etc.