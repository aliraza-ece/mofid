Notebook for MOF barcode system
========

Objective
---------
Decompose a MOF into its nodes, linkers, and topology to perform additional analysis (comparisons, chemoinformatics on its components, easy topology, etc.)


TODO
----
* OpenBabel does not currently handle PBC in bond perception.  Would need to make a new ConnectTheBonds with periodic boundaries for CIFs (see mol.cpp:3534).
	* See about adding/modifying a distance method within OBAtom/Bond.
	* Could also modify the FractionalWrapping type methods in OBUnitCell (generic.cpp).  Then could define distances based on wrapping, etc.
	* Also see GetAngle in mol.h as a generic distance, etc.
	* Consider editing the BeginModify and EndModify methods (as well as constructors/destructors) of OBMol to add UC data to the atoms and/or bonds.  This seems idiomatic (see mol.cpp:1500ish).
* Modifying bonds
	* Vector3 is based on doubles.  Is there an integer version for _direction or something similar?
	* Within the atom class, consider referring to the parent OBMol to determine PBC.  Might need to use the IsPeriodic or similar as a flag.
* Also need to modify elements.txt (and maxbonds) to keep the oxygen with tetrahedral bonding, etc, probably as an option to the CIF parser.  One idea would be to allow BABEL_DATADIR to accept multiple directories (though there may need to be an audit or debug message about that)


Ideas
-----
* See also <http://drivendata.github.io/cookiecutter-data-science> for project organization ideas


Notes
-----
Generated the eclipse code hinting by running `cmake -G "Eclipse CDT4 - Unix Makefiles" ../src` from within bin/ based on a [helpful post](http://stackoverflow.com/questions/11645575/importing-a-cmake-project-into-eclipse-cdt).  Another [website](http://www.badprog.com/c-eclipse-installation-of-c-c-development-tools-cdt-and-cygwin-for-windows) might be useful if I try to get the "Run" button working later.

It looks like there's a OBDistanceGeometry class, but in hindsight it's implementing bond distance/angle constraints, not really periodic boundaries.  However, there is already a mechanism for storing unit cell data within OBMol: see distgeom.cpp:163, which uses GetData (which in turn saves an OBUnitCell class within mol._vdata).

The [main API reference](http://openbabel.org/dev-api/namespaceOpenBabel.shtml) and [OBMol class reference](http://openbabel.org/dev-api/classOpenBabel_1_1OBMol.shtml) are also great resources.

### OpenBabel listserv



Table of contents
-----------------
* openbabel: Main openbabel distribution from Github on 6/23/16, commit 03fa0761dbd3b10a6d31036753faf8dddeda7ac5.  See <http://openbabel.org/wiki/Install_(Cygwin)> for installation instructions and <http://openbabel.org/wiki/Developer:Cpp_Tutorial> for the C++ tutorial.


Reading
-------
* 

