/* Copyright (C) 2012,2013 IBM Corp.
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
/* EncryptedArray.h  - Data-movement operations on arrays of slots
 */
#ifndef _EncryptedArray_H_
#define _EncryptedArray_H_

#include "FHE.h"
#include <NTL/ZZ_pX.h>
#include <NTL/GF2X.h>
#include <NTL/ZZX.h>


/****************************************************************

class: EncryptedArray

An object ea of type EncryptedArray stores information about an
FHEcontext context, and a monic polynomial G.  If context defines
parameters m, p, and r, then ea is a helper abject
that supports encoding/decoding and encryption/decryption
of vectors of plaintext slots over the ring (Z/(p^r)[X])/(G). 

The polynomial G should be irreducble over Z/(p^r) (this is not checked).
The degree of G should divide the multiplicative order of p modulo m
  (this is checked).

Currently, the following restriction is imposed:
either r == 1 or deg(G) == 1 or G == factors[0].

ea stores objects in the polynomial the polynomial ring Z/(p^r)[X].

Just as for the class PAlegebraMod,
if p == 2 and r == 1, then these polynomials
are represented as GF2X's, and otherwise as zz_pX's.
Thus, the types of these objects are not determined until run time.
As such, we need to use a class heirarchy, which mirrors
that of PAlgebraMod, as follows.

EncryptedArrayBase is a virtual class

EncryptedArrayDerived<type> is a derived template class, where
type is either PA_GF2 or PA_zz_p.

The class EncryptedArray is a simple wrapper around a smart pointer to an
EncryptedArrayBase object: copying an EncryptedArray object results is
a "deep copy" of the underlying object of the derived class.


****************************************************************/


class PlaintextArray; // forward reference

class EncryptedArrayBase {  
// purely abstract interface 

public:
  virtual ~EncryptedArrayBase() {}

  virtual EncryptedArrayBase* clone() const = 0;
  // makes this usable with cloned_ptr


  virtual const FHEcontext& getContext() const = 0;
  virtual const long getDegree() const = 0;

  virtual void rotate(Ctxt& ctxt, long k) const = 0; 
  // Rotation/shift as a linear array

  virtual void shift(Ctxt& ctxt, long k) const = 0;
  // non-cyclic shift with zero fill

  virtual void rotate1D(Ctxt& ctxt, long i, long k, bool dc=false) const = 0; 
  // rotate k positions along the i'th dimension
  // dc means "don't care", which means that the caller
  // guarantees that only zero elements rotate off the end --
  // this allows for some optimizations that would not otherwise
  // be possible

  virtual void shift1D(Ctxt& ctxt, long i, long k) const = 0; 
  // shift k positions along the i'th dimension with zero fill


  virtual void encode(ZZX& ptxt, const vector< long >& array) const = 0;
  virtual void encode(ZZX& ptxt, const vector< ZZX >& array) const = 0;
  virtual void encode(ZZX& ptxt, const PlaintextArray& array) const = 0;
  virtual void decode(vector< long  >& array, const ZZX& ptxt) const = 0;
  virtual void decode(vector< ZZX  >& array, const ZZX& ptxt) const = 0;
  virtual void decode(PlaintextArray& array, const ZZX& ptxt) const = 0;
  // encode/decode arrays into plaintext polynomials

  virtual void encodeUnitSelector(ZZX& ptxt, long i) const = 0;
  // encodes the vector with 1 at position i and 0 everywhere else


  virtual void encrypt(Ctxt& ctxt, const FHEPubKey& pKey, const vector< long >& ptxt) const = 0;
  virtual void encrypt(Ctxt& ctxt, const FHEPubKey& pKey, const vector< ZZX >& ptxt) const = 0;
  virtual void encrypt(Ctxt& ctxt, const FHEPubKey& pKey, const PlaintextArray& ptxt) const = 0;
  virtual void decrypt(const Ctxt& ctxt, const FHESecKey& sKey, vector< long >& ptxt) const = 0;
  virtual void decrypt(const Ctxt& ctxt, const FHESecKey& sKey, vector< ZZX >& ptxt) const = 0;
  virtual void decrypt(const Ctxt& ctxt, const FHESecKey& sKey, PlaintextArray& ptxt) const = 0;
  // encoding+encryption, decryption+decoding


  virtual void select(Ctxt& ctxt1, const Ctxt& ctxt2, const vector< long >& selector) const = 0;
  virtual void select(Ctxt& ctxt1, const Ctxt& ctxt2, const vector< ZZX >& selector) const = 0;
  virtual void select(Ctxt& ctxt1, const Ctxt& ctxt2, const PlaintextArray& selector) const = 0;
  // ctxt1 = ctxt1*selector + ctxt2*(1-selector)


  /* Linearized polynomials */

  virtual void buildLinPolyCoeffs(vector<ZZX>& C, const vector<ZZX>& L) const = 0;
  // L describes a linear map M by describing its action on
  //    the standard power basis: M(x^j mod G) = (L[j] mod G),
  //    for j = 0..d-1.  
  // The result is a coefficient vector C for the linearized
  //    polynomial representing M:   for h in Z/(p^r)[X] of degree < d,  
  //    M(h(X) mod G) = sum_{i=0}^{d-1} (C[j] mod G) * (h(X^{p^j}) mod G).


  /* some non-virtual convenience functions */

  long size() const { return getContext().zMStar.getNSlots(); } 
  // total size of hypercube

  long dimension() const { return getContext().zMStar.numOfGens(); }
  // number of dimensions of hypercube

  long sizeOfDimension(long i) const {return getContext().zMStar.OrderOf(i);}
  // size of given dimension

  long nativeDimension(long i) const {return getContext().zMStar.SameOrd(i);}
  // native rotations in given dimension?

  long coordinate(long i, long k) const {return getContext().zMStar.coordinate(i, k); }
  // returns ith coordinate of index k along the i'th dimension


};



template<class type> class EncryptedArrayDerived : public EncryptedArrayBase {
public:
  PA_INJECT(type)

private:
  const FHEcontext& context;
  MappingData<type> mappingData;


public:
  explicit
  EncryptedArrayDerived(const FHEcontext& _context, const RX& _G = RX(1, 1));

  EncryptedArrayDerived(const EncryptedArrayDerived& other) // copy constructor
  : context(other.context)
  {
    RBak bak; bak.save(); context.alMod.restoreContext();
    mappingData = other.mappingData;
  }

  EncryptedArrayDerived& operator=(const EncryptedArrayDerived& other) // assignment
  {
    if (this == &other) return *this;
    assert(&context == &other.context);

    RBak bak; bak.save(); context.alMod.restoreContext();
    mappingData = other.mappingData;
    return *this;
  }

  virtual EncryptedArrayBase* clone() const { return new EncryptedArrayDerived(*this); }

  const RX& getG() const { return mappingData.getG(); }

  virtual const FHEcontext& getContext() const { return context; }
  virtual const long getDegree() const { return mappingData.getDegG(); }



  virtual void rotate(Ctxt& ctxt, long k) const;
  virtual void shift(Ctxt& ctxt, long k) const;
  virtual void rotate1D(Ctxt& ctxt, long i, long k, bool dc=false) const;
  virtual void shift1D(Ctxt& ctxt, long i, long k) const;

  virtual void encode(ZZX& ptxt, const vector< long >& array) const
    { genericEncode(ptxt, array); }

  virtual void encode(ZZX& ptxt, const vector< ZZX >& array) const
    { genericEncode(ptxt, array); }

  virtual void encode(ZZX& ptxt, const PlaintextArray& array) const;

  virtual void encodeUnitSelector(ZZX& ptxt, long i) const;

  virtual void decode(vector< long  >& array, const ZZX& ptxt) const
    { genericDecode(array, ptxt); }

  virtual void decode(vector< ZZX  >& array, const ZZX& ptxt) const
    { genericDecode(array, ptxt); }

  virtual void decode(PlaintextArray& array, const ZZX& ptxt) const;

  virtual void encrypt(Ctxt& ctxt, const FHEPubKey& pKey, const vector< long >& ptxt) const
    { genericEncrypt(ctxt, pKey, ptxt); }

  virtual void encrypt(Ctxt& ctxt, const FHEPubKey& pKey, const vector< ZZX >& ptxt) const
    { genericEncrypt(ctxt, pKey, ptxt); }

  virtual void encrypt(Ctxt& ctxt, const FHEPubKey& pKey, const PlaintextArray& ptxt) const
    { genericEncrypt(ctxt, pKey, ptxt); }

  virtual void decrypt(const Ctxt& ctxt, const FHESecKey& sKey, vector< long >& ptxt) const
    { genericDecrypt(ctxt, sKey, ptxt); }

  virtual void decrypt(const Ctxt& ctxt, const FHESecKey& sKey, vector< ZZX >& ptxt) const
    { genericDecrypt(ctxt, sKey, ptxt); }

  virtual void decrypt(const Ctxt& ctxt, const FHESecKey& sKey, PlaintextArray& ptxt) const
    { genericDecrypt(ctxt, sKey, ptxt); }

  virtual void select(Ctxt& ctxt1, const Ctxt& ctxt2, const vector< long >& selector) const
    { genericSelect(ctxt1, ctxt2, selector); }

  virtual void select(Ctxt& ctxt1, const Ctxt& ctxt2, const vector< ZZX >& selector) const
    { genericSelect(ctxt1, ctxt2, selector); }

  virtual void select(Ctxt& ctxt1, const Ctxt& ctxt2, const PlaintextArray& selector) const
    { genericSelect(ctxt1, ctxt2, selector); }

  /* the following are specialized methods, used to work over extension fields...they assume 
     the modulus context is already set
   */

  void encode(ZZX& ptxt, const vector< RX >& array) const;
  void decode(vector< RX  >& array, const ZZX& ptxt) const;

  void encrypt(Ctxt& ctxt, const FHEPubKey& pKey, const vector< RX >& ptxt) const
    { genericEncrypt(ctxt, pKey, ptxt); }

  void decrypt(const Ctxt& ctxt, const FHESecKey& sKey, vector< RX >& ptxt) const
    { genericDecrypt(ctxt, sKey, ptxt); }

  void buildLinPolyCoeffs(vector<ZZX>& C, const vector<ZZX>& L) const;


private:

  /* helper template methods, to avoid repetitive code */

  template<class T> 
  void genericEncode(ZZX& ptxt, const T& array) const
  {
    RBak bak; bak.save(); context.alMod.restoreContext();

    vector< RX > array1;
    convert(array1, array);
    encode(ptxt, array1);
  }

  template<class T>
  void genericDecode(T& array, const ZZX& ptxt) const
  {
    RBak bak; bak.save(); context.alMod.restoreContext();

    vector< RX > array1;
    decode(array1, ptxt);
    convert(array, array1);
  }

  template<class T>
  void genericEncrypt(Ctxt& ctxt, const FHEPubKey& pKey, 
                      const T& array) const
  {
    assert(&context == &ctxt.getContext());
    ZZX pp;
    encode(pp, array); // Convert the array of slots into a plaintext polynomial
    pKey.Encrypt(ctxt, pp); // encrypt the plaintext polynomial
  }

  template<class T>
  void genericDecrypt(const Ctxt& ctxt, const FHESecKey& sKey, 
                      T& array) const
  {
    assert(&context == &ctxt.getContext());
    ZZX pp;
    sKey.Decrypt(pp, ctxt);
    decode(array, pp);
  }

  template<class T>
  void genericSelect(Ctxt& ctxt1, const Ctxt& ctxt2,
			    const T& selector) const
  {
    if (&ctxt1 == &ctxt2) return; // nothing to do

    assert(&context == &ctxt1.getContext() && &context == &ctxt2.getContext());
    ZZX poly; 
    encode(poly,selector);                             // encode as polynomial
    DoubleCRT dcrt(poly, context, ctxt1.getPrimeSet());// convert to DoubleCRT

    ctxt1.multByConstant(dcrt); // keep only the slots with 1's

    dcrt -= 1;      // convert 1 => 0, 0 => -1
    Ctxt tmp=ctxt2; // a copy of ctxt2
    tmp.multByConstant(dcrt);// keep (but negate) only the slots with 0's
    ctxt1 -= tmp;
  }
};


EncryptedArrayBase* buildEncryptedArray(const FHEcontext& context, const ZZX& G);
//  A "factory" for building EncryptedArrays



class EncryptedArray {  
// A simple wrapper for a smart pointer to an EncryptedArrayBase...
// This is the interface that higher-level code should use

private:
  cloned_ptr<EncryptedArrayBase> rep;

public:

  EncryptedArray(const FHEcontext& context, const ZZX& G = ZZX(1, 1))
  : rep(buildEncryptedArray(context, G))
  { }
  // constructor: G defaults to the monomial X

  // copy constructor: default
  // assignment: default

  template<class type> 
  const EncryptedArrayDerived<type>& getDerived(type) const
  { return dynamic_cast< const EncryptedArrayDerived<type>& >( *rep ); }
  // downcast operator
  // example:  const EncryptedArrayDerived<PA_GF2>& rep = ea.getDerived(PA_GF2());


  /* direct access to EncryptedArrayBase methods */

  const FHEcontext& getContext() const { return rep->getContext(); }
  const long getDegree() const { return rep->getDegree(); }
  void rotate(Ctxt& ctxt, long k) const { rep->rotate(ctxt, k); }
  void shift(Ctxt& ctxt, long k) const { rep->shift(ctxt, k); }
  void rotate1D(Ctxt& ctxt, long i, long k, bool dc=false) const { rep->rotate1D(ctxt, i, k, dc); }
  void shift1D(Ctxt& ctxt, long i, long k) const { rep->shift1D(ctxt, i, k); }


  void encode(ZZX& ptxt, const vector< long >& array) const 
    { rep->encode(ptxt, array); }
  void encode(ZZX& ptxt, const vector< ZZX >& array) const 
    { rep->encode(ptxt, array); }
  void encode(ZZX& ptxt, const PlaintextArray& array) const 
    { rep->encode(ptxt, array); }

  void encodeUnitSelector(ZZX& ptxt, long i) const
    { rep->encodeUnitSelector(ptxt, i); }

  void decode(vector< long  >& array, const ZZX& ptxt) const 
    { rep->decode(array, ptxt); }
  void decode(vector< ZZX  >& array, const ZZX& ptxt) const 
    { rep->decode(array, ptxt); }
  void decode(PlaintextArray& array, const ZZX& ptxt) const 
    { rep->decode(array, ptxt); }

  void encrypt(Ctxt& ctxt, const FHEPubKey& pKey, const vector< long >& ptxt) const 
    { rep->encrypt(ctxt, pKey, ptxt); }
  void encrypt(Ctxt& ctxt, const FHEPubKey& pKey, const vector< ZZX >& ptxt) const 
    { rep->encrypt(ctxt, pKey, ptxt); }
  void encrypt(Ctxt& ctxt, const FHEPubKey& pKey, const PlaintextArray& ptxt) const 
    { rep->encrypt(ctxt, pKey, ptxt); }


  void decrypt(const Ctxt& ctxt, const FHESecKey& sKey, vector< long >& ptxt) const 
    { rep->decrypt(ctxt, sKey, ptxt); }
  void decrypt(const Ctxt& ctxt, const FHESecKey& sKey, vector< ZZX >& ptxt) const 
    { rep->decrypt(ctxt, sKey, ptxt); }
  void decrypt(const Ctxt& ctxt, const FHESecKey& sKey, PlaintextArray& ptxt) const
    { rep->decrypt(ctxt, sKey, ptxt); }


  void select(Ctxt& ctxt1, const Ctxt& ctxt2, const vector< long >& selector) const 
    { rep->select(ctxt1, ctxt2, selector); }
  void select(Ctxt& ctxt1, const Ctxt& ctxt2, const vector< ZZX >& selector) const 
    { rep->select(ctxt1, ctxt2, selector); }
  void select(Ctxt& ctxt1, const Ctxt& ctxt2, const PlaintextArray& selector) const
    { rep->select(ctxt1, ctxt2, selector); }


  void buildLinPolyCoeffs(vector<ZZX>& C, const vector<ZZX>& L) const
    { rep->buildLinPolyCoeffs(C, L); }


  long size() const { return rep->size(); } 
  long dimension() const { return rep->dimension(); }
  long sizeOfDimension(long i) const { return rep->sizeOfDimension(i); }
  long nativeDimension(long i) const {return rep->nativeDimension(i); }
  long coordinate(long i, long k) const { return rep->coordinate(i, k); }

};




/****************************************************************

class: PlaintextArray

An object pa of type PlaintextArray stores information
about an EncryptedArray object ea.  The object pa stores
a vector of plaintext slots, where each slot is an element
of the polynomial ring (Z/(p^r)[X])/(G), where p, r, and G
are as defined in ea.  Support for arithemetic on PlaintextArray
objects is provided.

Mirroring PAlgebraMod and EncryptedArray, we have the following
class heirarchy:


PlaintextArrayBase is a virtual class

PlaintextArrayDerived<type> is a derived template class, where
type is either PA_GF2 or PA_zz_p.

The class PlaintextArray is a simple wrapper around a smart pointer to a
PlaintextArray object: copying a PlaintextArray object results is
a "deep copy" of the underlying object of the derived class.



****************************************************************/



class PlaintextArrayBase { 
// purely abstract interface

public:
  virtual ~PlaintextArrayBase() {}

  virtual PlaintextArrayBase* clone() const = 0;
  // makes this usable with cloned_ptr

  virtual const EncryptedArray& getEA() const = 0;

  virtual void rotate(long k) = 0; 
  // Rotation/shift as a linear array

  virtual void shift(long k) = 0;
  // non-cyclic shift with zero fill

  virtual void encode(const vector< long >& array) = 0;
  virtual void encode(const vector< ZZX >& array) = 0;
  virtual void decode(vector< long  >& array) const = 0;
  virtual void decode(vector< ZZX  >& array) const = 0;
  // encode/decode arrays into plaintext polynomials

  virtual void encode(long val) = 0;
  virtual void encode(const ZZX& val) = 0;
  // encode with the same value replicated in each slot

  virtual void random() = 0;
  // generate random

  virtual bool equals(const PlaintextArrayBase& other) const = 0;
  virtual bool equals(const vector<long>& other) const = 0;
  virtual bool equals(const vector<ZZX>& other) const = 0;
  // equality testing

  virtual void add(const PlaintextArrayBase& other) = 0;
  virtual void sub(const PlaintextArrayBase& other) = 0;
  virtual void mul(const PlaintextArrayBase& other) = 0;
  virtual void negate() = 0;
  // arithmetic

  virtual void replicate(long i) = 0;
  // replicate coordinate i at all coordinates

  virtual void print(ostream& s) const = 0;
  // output

};


template<class type> class PlaintextArrayDerived : public PlaintextArrayBase {
public:
  PA_INJECT(type)

private:
  const EncryptedArray& ea;
  vector< RX > data;

  /* the following are just for convenience */
  const PAlgebraModDerived<type>& tab;
  const RX& G;
  long degG;
  long n;

public:

  virtual PlaintextArrayBase* clone() const { return new PlaintextArrayDerived(*this); }
  virtual const EncryptedArray& getEA() const { return ea; }

  PlaintextArrayDerived(const EncryptedArray& _ea) : 
    ea(_ea),
    tab( ea.getContext().alMod.getDerived(type()) ),
    G( ea.getDerived(type()).getG() )
  {
    RBak bak; bak.save(); tab.restoreContext();

    degG = deg(G);
    n = ea.size();
    data.resize(n);
  }

  PlaintextArrayDerived(const PlaintextArrayDerived& other) // copy constructor
  : ea(other.ea), tab(other.tab), G(other.G), degG(other.degG), n(other.n) 
  {
    RBak bak; bak.save(); tab.restoreContext();
    data = other.data;
  }


  PlaintextArrayDerived& operator=(const PlaintextArrayDerived& other) // assignment
  {
    if (this == &other) return *this;
    assert(&ea == &other.ea); 
    RBak bak; bak.save(); tab.restoreContext();
    data = other.data;
    return *this;
  }

  virtual void rotate(long k) 
  {
    RBak bak; bak.save(); tab.restoreContext();

    vector<RX> tmp(n); 

    for (long i = 0; i < n; i++)
      tmp[((i+k)%n + n)%n] = data[i];

    data = tmp;
  }

  virtual void shift(long k) 
  {
    RBak bak; bak.save(); tab.restoreContext();

    for (long i = 0; i < n; i++)
      if (i + k >= n || i + k < 0)
        clear(data[i]);

    rotate(k);
  }

  virtual void encode(const vector< long >& array) 
  {
    assert(lsize(array) == n);
    RBak bak; bak.save(); tab.restoreContext();
    convert(data, array);
  }

  virtual void encode(const vector< ZZX >& array) 
  {
    assert(lsize(array) == n);
    RBak bak; bak.save(); tab.restoreContext();
    convert(data, array);
    for (long i = 0; i < lsize(array); i++) assert(deg(data[i]) < degG);
  }

  virtual void decode(vector< long  >& array) const
  {
    RBak bak; bak.save(); tab.restoreContext();
    convert(array, data);
  }

  virtual void decode(vector< ZZX  >& array) const 
  {
    RBak bak; bak.save(); tab.restoreContext();
    convert(array, data);
  }

  virtual void encode(long val)
  {
    vector<long> array;
    array.resize(n);
    for (long i = 0; i < n; i++) array[i] = val;
    encode(array);
  }

  virtual void encode(const ZZX& val)
  {
    vector<ZZX> array;
    array.resize(n);
    for (long i = 0; i < n; i++) array[i] = val;
    encode(array);
  }

  virtual void random() 
  {
    RBak bak; bak.save(); tab.restoreContext();
    for (long i = 0; i < n; i++)
      NTL::random(data[i], degG);
  }

  virtual bool equals(const PlaintextArrayBase& other) const 
  {
    RBak bak; bak.save(); tab.restoreContext();
    const PlaintextArrayDerived<type>& other1 = dynamic_cast< const PlaintextArrayDerived<type>& >( other );

    assert(&ea == &other1.ea);

    return data == other1.data;
  }

  virtual bool equals(const vector<long>& other) const 
  {
    RBak bak; bak.save(); tab.restoreContext();
    vector<RX> tmp;
    convert(tmp, other);
    return data == tmp;
  }

  virtual bool equals(const vector<ZZX>& other) const 
  {
    RBak bak; bak.save(); tab.restoreContext();
    vector<RX> tmp;
    convert(tmp, other);
    return data == tmp;
  }

  virtual void add(const PlaintextArrayBase& other) 
  {
    RBak bak; bak.save(); tab.restoreContext();
    const PlaintextArrayDerived<type>& other1 = 
      dynamic_cast< const PlaintextArrayDerived<type>& >( other );

    assert(&ea == &other1.ea);

    for (long i = 0; i < n; i++) 
      data[i] += other1.data[i];
  }

  virtual void sub(const PlaintextArrayBase& other) 
  {
    RBak bak; bak.save(); tab.restoreContext();
    const PlaintextArrayDerived<type>& other1 = 
      dynamic_cast< const PlaintextArrayDerived<type>& >( other );

    assert(&ea == &other1.ea);

    for (long i = 0; i < n; i++) 
      data[i] -= other1.data[i];
  }


  virtual void mul(const PlaintextArrayBase& other) 
  {
    RBak bak; bak.save(); tab.restoreContext();
    const PlaintextArrayDerived<type>& other1 = 
      dynamic_cast< const PlaintextArrayDerived<type>& >( other );

    assert(&ea == &other1.ea);

    for (long i = 0; i < n; i++) 
      MulMod(data[i], data[i], other1.data[i], G);
  }


  virtual void negate() 
  {
    RBak bak; bak.save(); tab.restoreContext();
    for (long i = 0; i < n; i++) 
      NTL::negate(data[i], data[i]);
  }

  virtual void replicate(long i)
  {
    RBak bak; bak.save(); tab.restoreContext();

    assert(i >= 0 && i < n);
    for (long j = 0; j < n; j++) {
      if (j != i) data[j] = data[i];
    }
  }

  virtual void print(ostream& s) const 
  {
    if (n == 0) 
      s << "[]";
    else {
      s << "[" << data[0];
      for (long i = 1; i < lsize(data); i++)
        s << " " << data[i];
      s << "]";
    }
  }



  /* The follwing two methods assume that the modulus context is already set */

  const vector<RX>& getData() const { return data; }

  void setData(const vector<RX>& _data) 
  {
    assert(lsize(_data) == n);
    data = _data;
  }


};


PlaintextArrayBase* buildPlaintextArray(const EncryptedArray& ea);
//  A "factory" for building EncryptedArrays


class PlaintextArray { 
// a simple wrapper for a pointer to a PlaintextArrayBase.
// This is the interface that higher-level code should use

private:
  cloned_ptr<PlaintextArrayBase> rep;

public:

  PlaintextArray(const EncryptedArray& ea)
  : rep(buildPlaintextArray(ea))
  { }
  // constructor

  // copy constructor: default
  // assignment: default


  template<class type> 
  const PlaintextArrayDerived<type>& getDerived(type) const
  { return dynamic_cast< const PlaintextArrayDerived<type>& >( *rep ); }

  template<class type> 
  PlaintextArrayDerived<type>& getDerived(type) 
  { return dynamic_cast< PlaintextArrayDerived<type>& >( *rep ); }

  // downcast operators (differeing only by const)
  // example:  const PlaintextArrayDerived<PA_GF2>& rep = pa.getDerived(PA_GF2());


  /* direct access to PlaintextArrayBase methods */

  const EncryptedArray& getEA() const { return rep->getEA(); }

  void rotate(long k) { rep->rotate(k); }
  void shift(long k) { rep->shift(k); }

  void encode(const vector< long >& array) { rep->encode(array); }
  void encode(const vector< ZZX >& array) { rep->encode(array); }

  void decode(vector< long  >& array) { rep->decode(array); }
  void decode(vector< ZZX  >& array) { rep->decode(array); }

  void encode(long val) { rep->encode(val); }
  void encode(const ZZX& val) { rep->encode(val); }

  void random() { rep->random(); }

  bool equals(const PlaintextArray& other) const { return rep->equals(*other.rep); }
  bool equals(const vector<long>& other) const { return rep->equals(other); }
  bool equals(const vector<ZZX>& other) const { return rep->equals(other); }


  void add(const PlaintextArray& other) { rep->add(*other.rep); }
  void sub(const PlaintextArray& other) { rep->sub(*other.rep); }
  void mul(const PlaintextArray& other) { rep->mul(*other.rep); }
  void negate() { rep->negate(); }
  void replicate(long i) { rep->replicate(i); }

  void print(ostream& s) const { rep->print(s); }


};





#endif /* ifdef _EncryptedArray_H_ */
