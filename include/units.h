/* 
 * File:   units.h
 * Author: vehoff
 *
 * Created on January 21, 2010, 12:07 PM
 */

#ifndef _UNITS_H
#define	_UNITS_H

/// defines all constants necessary to convert from MD to QM units and back
static const double RA=0.529189379; /// the Bohr radius in [Ang]
static const double hbar_eV=6.58211899e-16; /// Planck constant in [eV*s]
static const double Hartree =27.211396132; /// [Hartree] to [eV]

/// units allowed in conversion
enum {
    nm=0,
    bohr,
    eV,
    hartree,
    m
};

/// template for compile time assertion required for unit conversion class
template <bool t>
struct ctassert {
  enum { N = 1 - 2 * int(!t) };
  /// 1 if t is true, -1 if t is false.
  static char A[N];
  /// ***NOTICE: If this error occurs you tried to use an undefined conversion.***
};


/// class for unit conversion
template<int,int>
class unit{

public:

    template <typename T>
    static T to(T d){  // is only called in case of an unspecified conversion
        ctassert<false> foo;
        return T();
    }
};

/// macro to allow easy conversion definitions
#define DEFINE_CONVERSION(a, b, c) \
template <> \
class unit <a,b> {\
public: \
    template <typename T>\
    static T to(T value){\
        return c;\
    }\
};

/// definition of unit conversions
DEFINE_CONVERSION(nm, bohr, value*10./RA);
DEFINE_CONVERSION(bohr, nm, value*RA/10.);
DEFINE_CONVERSION(eV, hartree, value/Hartree);
DEFINE_CONVERSION(hartree, eV, value*Hartree);
DEFINE_CONVERSION(m, nm, value*1E+9);
DEFINE_CONVERSION(nm, m, value*1E-9);

#endif /* _UNITS_H */