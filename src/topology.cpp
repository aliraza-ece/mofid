#include "topology.h"
#include "framework.h"
#include "virtual_mol.h"
#include "pseudo_atom.h"
#include "periodic.h"
#include "obdetails.h"

#include <vector>
#include <string>
#include <set>
#include <tuple>
#include <queue>

#include <openbabel/babelconfig.h>
#include <openbabel/mol.h>
#include <openbabel/atom.h>
#include <openbabel/obiter.h>
#include <openbabel/generic.h>


namespace OpenBabel
{

bool AtomRoles::HasRole(const std::string &test_role) {
	return (_roles.find(test_role) != _roles.end());
}

void AtomRoles::ClearRoles() {
	_roles.clear();
}

void AtomRoles::AddRole(const std::string &possibly_new_role) {
	_roles.insert(possibly_new_role);
}

bool AtomRoles::RemoveRole(const std::string &role) {
	bool deleted = false;
	if (HasRole(role)) {
		_roles.erase(role);
	}
	return deleted;
}


ConnectionTable::ConnectionTable(OBMol* parent) {
	parent_net = parent;
}

void ConnectionTable::AddConn(PseudoAtom conn, PseudoAtom begin, PseudoAtom end) {
	if (begin == end) { return; }  // trivial loop: TODO raise ERROR
	std::pair<PseudoAtom, PseudoAtom> endpoints(begin, end);
	conn2endpts[conn] = endpoints;
	endpt_conns[begin].insert(conn);
	endpt_conns[end].insert(conn);
}

void ConnectionTable::RemoveConn(PseudoAtom conn) {
	std::pair<PseudoAtom, PseudoAtom> endpoints;
	endpoints = conn2endpts[conn];
	conn2endpts.erase(conn);
	endpt_conns.erase(endpoints.first);
	endpt_conns.erase(endpoints.second);
}

bool ConnectionTable::IsConn(PseudoAtom atom) {
	return (conn2endpts.find(atom) == conn2endpts.end());
}

AtomSet ConnectionTable::GetAtomConns(PseudoAtom endpt) {
	return endpt_conns[endpt];
}

bool ConnectionTable::HasNeighbor(PseudoAtom begin, PseudoAtom end) {
	AtomSet begin_conn = GetAtomConns(begin);
	for (AtomSet::iterator it=begin_conn.begin(); it!=begin_conn.end(); ++it) {
		if ((conn2endpts[*it].first == end) || (conn2endpts[*it].second == end)) {
			return true;
		}
	}
	return false;
}

AtomSet ConnectionTable::GetConnEndpoints(PseudoAtom conn) {
	std::set<PseudoAtom> endpoints;
	endpoints.insert(conn2endpts[conn].first);
	endpoints.insert(conn2endpts[conn].second);
	return endpoints;
}

VirtualMol ConnectionTable::GetInternalConns(VirtualMol atoms) {
	if (atoms.GetParent() != parent_net) { return VirtualMol(NULL); };  // error
	VirtualMol int_conn(parent_net);
	AtomSet pa = atoms.GetAtoms();
	for (AtomSet::iterator a_it=pa.begin(); a_it!=pa.end(); ++a_it) {
		AtomSet pa_conns = endpt_conns[*a_it];
		for (AtomSet::iterator conn_it=pa_conns.begin(); conn_it!=pa_conns.end(); ++conn_it) {
			std::pair<PseudoAtom, PseudoAtom> endpoints = conn2endpts[*conn_it];
			if (atoms.HasAtom(endpoints.first) && atoms.HasAtom(endpoints.second)) {
				// Only add connections if both endpoints are internal
				int_conn.AddAtom(*conn_it);
			}
		}
	}
	return int_conn;
}

/*
bool ConnectionTable::CheckConsistency() {
	// This would be a nice feature eventually
	// number of bonds upstream must equal number of connections.
	// also the two internal databases should match
	return true;
}
*/


Topology::Topology(OBMol *parent_mol) {
	orig_molp = parent_mol;
	simplified_net = initMOFwithUC(parent_mol);

	ConnectionTable conns(&simplified_net);
	VirtualMol deleted_atoms(orig_molp);
	PseudoAtomMap pa_to_act(&simplified_net, orig_molp);
	std::map<OBAtom*, AtomRoles> act_roles();    // initialize to empty.  Automatically will add elements

	// Initialize simplified_net via copying orig_mol and creating the 1:1 mapping
	FOR_ATOMS_OF_MOL(orig_atom, *orig_molp) {
		OBAtom* new_atom;
		new_atom = formAtom(&simplified_net, orig_atom->GetVector(), DEFAULT_ELEMENT);
		pa_to_act[new_atom].AddAtom(&*orig_atom);
		act_to_pa[&*orig_atom] = new_atom;
	}
	// Bonds in the simplified net are handled specially with a shadow ConnectionTable object
	FOR_BONDS_OF_MOL(orig_bond, *orig_molp) {
		PseudoAtom begin_pa = act_to_pa[orig_bond->GetBeginAtom()];
		PseudoAtom end_pa = act_to_pa[orig_bond->GetEndAtom()];
		ConnectAtoms(begin_pa, end_pa);
	}
}

bool Topology::IsConnection(PseudoAtom a) {
	return conns.IsConn(a);
}

VirtualMol Topology::GetOrigAtomsOfRole(const std::string &role) {
	VirtualMol match(orig_molp);
	for (std::map<OBAtom*, AtomRoles>::iterator it=act_roles.begin(); it!=act_roles.end(); ++it) {
		if (it->second.HasRole(role)) {
			match.AddAtom(it->first);
		}
	}
	return match;
}

void Topology::SetRoleToAtoms(const std::string &role, VirtualMol atoms, bool val) {
	// Adds/removes the role from a list of original atoms in a VirtualMol
	if (atoms.GetParent() != orig_molp) {
		// Error
		return;
	}
	std::set<OBAtom*> atom_list = atoms.GetAtoms();
	for (std::set<OBAtom*>::iterator it=atom_list.begin(); it!=atom_list.end(); ++it) {
		if (val) {
			act_roles[*it].AddRole(role);
		} else {
			act_roles[*it].RemoveRole(role);
		}
	}
}

int Topology::RemoveOrigAtoms(VirtualMol atoms) {
	// FIXME: come back and implement at the end
	// Used for solvent removal, etc.
	if (atoms.GetParent() != orig_molp) {
		return 0;  // error
	}
	std::set<OBAtom*> atom_set = atoms.GetAtoms();
	for (std::set<OBAtom*>::iterator it=atom_set.begin(); it!=atom_set.end(); ++it) {
		deleted_atoms.AddAtom(*it);
		// delete from simplified net
		// other accounting to take care of?

	}
	return 1;
	// remove from the simplified net
	//deleted_atoms
}

PseudoAtom Topology::ConnectAtoms(PseudoAtom begin, PseudoAtom end, vector3 *pos) {
	// Form the pseudo atom
	// It's not a problem if they're already connected.
	// In fact it's the whole point for structures like MOF-5, and the ConnectionTable is built to handle it.
	// As such, the valence of a pseudoatom will be it's OBAtom valence or size of ConnectionTable::GetAtomConns
	vector3 conn_pos;
	if (pos != NULL) {
		conn_pos = *pos;
	} else {  // use the midpoint by default
		OBUnitCell* uc = getPeriodicLattice(&simplified_net);
		vector3 begin_pos = begin->GetVector();
		vector3 end_pos = uc->UnwrapCartesianNear(end->GetVector(), begin_pos);

		conn_pos = uc->WrapCartesianCoordinate((begin_pos+end_pos) / 2.0);
	}
	PseudoAtom new_conn = formAtom(&simplified_net, conn_pos, CONNECTION_ELEMENT);

	// Form bonds and update accounting for ConnectionTable object
	formBond(&simplified_net, begin, new_conn, 1);
	formBond(&simplified_net, end, new_conn, 1);
	conns.AddConn(new_conn, begin, end);
	return new_conn;
}

void Topology::DeleteConnection(PseudoAtom conn) {
	// Removes connections between two atoms (no longer directly bonded through a connection site)
	conns.RemoveConn(conn);
	simplified_net.DeleteAtom(conn);  // automatically deletes attached bonds
}

void Topology::DeleteAtomAndConns(PseudoAtom atom) {
	// Removes an atom and relevant bonds/connections
	AtomSet nbors;
	FOR_NBORS_OF_ATOM(nbor, *atom) {
		if (!IsConnection(&*nbor)) {
			return;  // ERROR.  No atoms should be bonded directly to each other
		}
		nbors.insert(&*nbor);
	}
	for (AtomSet::iterator it=nbors.begin(); it!=nbors.end(); ++it) {
		DeleteConnection(*it);
	}
	simplified_net.DeleteAtom(atom);
}

ConnIntToExt Topology::GetConnectedAtoms(VirtualMol internal_pa) {
	// Gets the next shell of PseudoAtom neighbors external to the internal VirtualMol.
	// Automatically passes over connection pseudoatoms.

	// VirtualMol::GetExternalBonds() will also return internal connections, so let's specify those ahead of time
	VirtualMol pa_with_conns = internal_pa;
	pa_with_conns.AddVirtualMol(conns.GetInternalConns(internal_pa));
	// Then GetExternalBonds() can only return extra bonds
	ConnIntToExt bonds = pa_with_conns.GetExternalBonds();

	// Translate external connections to external pseudoatoms
	ConnIntToExt external_nbors;
	for (ConnIntToExt::iterator e_it=bonds.begin(); e_it!=bonds.end(); ++e_it) {
		PseudoAtom int_to_conn = e_it->first;
		PseudoAtom ext_conn = e_it->second;
		PseudoAtom ext_from_conn = NULL;
		AtomSet conn_endpts = conns.GetConnEndpoints(ext_conn);

		// Figure out which of the conn_endpts is the new external endpoint
		for (AtomSet::iterator c_it=conn_endpts.begin(); c_it!=conn_endpts.end(); ++c_it) {
			if (*c_it != int_to_conn) {
				ext_from_conn = *c_it;
			}
		}
		std::pair<OBAtom*, OBAtom*> ConnExt(int_to_conn, ext_from_conn);
		external_nbors.insert(ConnExt);
	}

	return external_nbors;
}

// TODO: consider renaming as CollapseFragment, using pseudo atoms and
// another public translator method?
// That will probably be less cobbled together and more reproducible.
PseudoAtom Topology::CollapseOrigAtoms(VirtualMol fragment) {
	// Simplifies the net by combining all original atoms specified in the fragment
	// into a single pseudo-atom, maintaining existing connections.
	// Returns the pointer to the generated pseudo atom.
	if (fragment.GetParent() != orig_molp) {
		return NULL;  // error
	}

	// Find the relevant set of pseudoatoms
	// We don't need to handle internal connections because atom manipulation will automatically take care of it.
	std::set<OBAtom*> act_atoms = fragment.GetAtoms();
	VirtualMol orig_pa(&simplified_net);
	for (std::set<OBAtom*>::iterator it=act_atoms.begin(); it!=act_atoms.end(); ++it) {
		orig_pa.AddAtom(act_to_pa[*it]);
	}
	// TODO consider a consistency check that the PA's don't include any other atoms (a length check for fragment vs. sum of PA AtomSets)

	// Get external neighbors on the other side of the connections.
	ConnIntToExt external_nbors = GetConnectedAtoms(orig_pa);

	// Make the new pseudoatom at the centroid
	OBMol mol_orig_pa = orig_pa.ToOBMol(false);  // we only need positions, not bonds
	vector3 centroid = getCentroid(&mol_orig_pa, false);  // without weights
	PseudoAtom new_atom = formAtom(&simplified_net, centroid, DEFAULT_ELEMENT);

	// Put the connection point 1/3 of the way between the centroid and the connection midpoint to the exterior
	// (e.g. 1/3 of the way between a BDC centroid and the O-M bond in the -COO group).
	// In a simplified M-X-X-M' system, this will have the convenient property of being mostly equidistant.
	// Note: this follows the convention of many top-down MOF generators placing the connection point halfway on the node-linker bond.
	// In this circumstance, the convention also has the benefit that a linker with many connections to the same metal (-COO)
	// or connections to multiple metals (MOF-74 series) have unique positions for the X_CONN pseudo atoms.
	for (ConnIntToExt::iterator it=external_nbors.begin(); it!=external_nbors.end(); ++it) {
		OBAtom* pa_int = it->first;
		OBAtom* pa_ext = it->second;

		// Positions of the internal/external bonding atoms, to get the bond location
		OBUnitCell* lattice = getPeriodicLattice(&simplified_net);
		vector3 int_loc = lattice->UnwrapCartesianNear(pa_int->GetVector(), centroid);
		vector3 ext_loc = lattice->UnwrapCartesianNear(pa_ext->GetVector(), int_loc);

		vector3 conn_loc = lattice->WrapCartesianCoordinate((4.0*centroid + int_loc + ext_loc) / 6.0);
		ConnectAtoms(new_atom, pa_ext, &conn_loc);
	}

	// Delete the original fragment pseudoatoms
	AtomSet orig_pa_set = orig_pa.GetAtoms();
	for (AtomSet::iterator it=orig_pa_set.begin(); it!=orig_pa_set.end(); ++it) {
		DeleteAtomAndConns(*it);
	}

	return new_atom;
}

OBMol Topology::ToOBMol() {
	// TODO: eventually this code will assign pseudo-atom types based on SMILES (like ElementGen),
	// but for now, just spit out the OBMol of interest

	// For the interim, let's try coloring the atoms as a test.
	// This will not likely be the implementation for the final version of the code, but it's worth trying now
	for (std::map<OBAtom*, AtomRoles>::iterator it=act_roles.begin(); it!=act_roles.end(); ++it) {
		PseudoAtom a = act_to_pa[it->first];
		if (it->second.HasRole("node")) {
			changeAtomElement(a, 40);  // Zr (teal)
		} else if (it->second.HasRole("linker")) {
			changeAtomElement(a, 7);  // N (blue)
		}
	}
	// I did the coloring this way out of convenience, but honestly it's actually
	// a really good way to visualize how the net turned out.
	// A web app to combine/hide the different layers could work too.
	return simplified_net;
}

} // end namespace OpenBabel
