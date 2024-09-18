/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#ifndef __SORT_H__
#define __SORT_H__

/*
================================================================================================
Contains the generic templated sort algorithms for quick-sort, heap-sort and insertion-sort.

The sort algorithms do not use class operators or overloaded functions to compare
objects because it is often desireable to sort the same objects in different ways
based on different keys (not just ascending and descending but sometimes based on
name and other times based on say priority). So instead, for each different sort a
separate class is implemented with a Compare() function.

This class is derived from one of the classes that implements a sort algorithm.
The Compare() member function does not only define how objects are sorted, the class
can also store additional data that can be used by the Compare() function. This, for
instance, allows a list of indices to be sorted where the indices point to objects
in an array. The base pointer of the array with objects can be stored on the class
that implements the Compare() function such that the Compare() function can use keys
that are stored on the objects.

The Compare() function is not virtual because this would incur significant overhead.
Do NOT make the Compare() function virtual on the derived class!
The sort implementations also explicitely call the Compare() function of the derived
class. This is to avoid various compiler bugs with using overloaded compare functions
and the inability of various compilers to find the right overloaded compare function.

To sort an array, an idList or an idStaticList, a new sort class, typically derived from
idSort_Quick, is implemented as follows:

class idSort_MySort : public idSort_Quick< idMyObject, idSort_MySort > {
public:
	int Compare( const idMyObject & a, const idMyObject & b ) const {
		if ( a should come before b ) {
			return -1; // or any negative integer
		} if ( a should come after b ) {
			return 1;  // or any positive integer
		} else {
			return 0;
		}
	}
};

To sort an array:

idMyObject array[100];
idSort_MySort().Sort( array, 100 );

To sort an idList:

idList< idMyObject > list;
list.Sort( idSort_MySort() );

The sort implementations never create temporaries of the template type. Only the
'SwapValues' template is used to move data around. This 'SwapValues' template can be
specialized to implement fast swapping of data. For instance, when sorting a list with
objects of some string class it is important to implement a specialized 'SwapValues' for
this string class to avoid excessive re-allocation and copying of strings.

================================================================================================
*/

#include "../../sys/platform.h"

/*
========================
SwapValues
========================
*/
template< typename _type_ >
ID_INLINE void SwapValues( _type_ & a, _type_ & b )
{
    _type_ c = a;
    a = b;
    b = c;
}
#endif