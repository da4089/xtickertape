/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 1995.
  Unpublished work.  All Rights Reserved.

  The software contained on this media is the property of the
  DSTC Pty Ltd.  Use of this software is strictly in accordance
  with the license agreement in the accompanying LICENSE.DOC
  file.  If your distribution of this software does not contain
  a LICENSE.DOC file then you have no rights to use this
  software in any manner and should contact DSTC at the address
  below to determine an appropriate licensing arrangement.

     DSTC Pty Ltd
     Level 7, Gehrmann Labs
     University of Queensland
     St Lucia, 4072
     Australia
     Tel: +61 7 3365 4310
     Fax: +61 7 3365 4311
     Email: enquiries@dstc.edu.au

  This software is being provided "AS IS" without warranty of
  any kind.  In no event shall DSTC Pty Ltd be liable for
  damage of any kind arising out of or in connection with
  the use or performance of this software.

****************************************************************/

#ifndef ATOM_H
#define ATOM_H

#ifndef lint
static const char cvs_ATOM_H[] = "$Id: atom.h,v 2.1 2000/11/01 02:07:02 phelps Exp $";
#endif /* lint */

/* The types of atoms */
typedef enum
{
    ATOM_NIL,
    ATOM_INT32,
    ATOM_INT64,
    ATOM_FLOAT,
    ATOM_STRING,
    ATOM_SYMBOL,
    ATOM_CONS
} atom_type_t;

/* An atom_t is a an opaque struct */
typedef struct atom *atom_t;


/* Answers the unique nil instance */
atom_t nil_alloc(elvin_error_t error);

/* Allocates and initializes a new symbol atom */
atom_t symbol_alloc(char *name, elvin_error_t error);

/* Allocates and initializes a new 32-bit integer atom */
atom_t int32_alloc(int32_t value, elvin_error_t error);

/* Allocates and initializes a new 64-bit integer atom */
atom_t int64_alloc(int64_t value, elvin_error_t error);

/* Allocates and initializes a new float atom */
atom_t float_alloc(double value, elvin_error_t error);

/* Allocates and initializes a new string atom */
atom_t string_alloc(uchar *value, elvin_error_t error);

/* Allocates and initializes a new cons atom */
atom_t cons_alloc(atom_t car, atom_t cdr, elvin_error_t error);


/* Returns the atom's type */
atom_type_t atom_get_type(atom_t atom);


/* Frees an atom */
int atom_free(atom_t atom, elvin_error_t error);

/* Allocates and returns a string representing the atom */
char *atom_to_string(atom_t atom, elvin_error_t error);

/* Evaluates an atom */
atom_t atom_eval(atom_t atom, elvin_error_t error);

/* Returns the symbol's name */
char *symbol_name(atom_t symbol);

/* Returns the integer's value */
int32_t int32_value(atom_t integer);

/* Returns the integer's value */
int64_t int64_value(atom_t long_int);

/* Returns the string's bytes */
uchar *string_value(atom_t string);

/* Answers the car of a cons atom */
atom_t cons_car(atom_t cons);

/* Answers the cdr of a cons atom */
atom_t cons_cdr(atom_t cons);

/* Reverse the elements of a list */
atom_t cons_reverse(atom_t cons, atom_t end);

#endif /* ATOM_H */
