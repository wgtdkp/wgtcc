#ifndef _WGTCC_TAC_H_
#define _WGTCC_TAC_H_

#include <cinttypes>

using TACType = uint8_t;


/*
 * Three Adress Code
 *  constant    64bit integer, floating numbers are converted.
 *  variable    local/global, width, integer/float/aggregate.
 *  temporary   intermediate values, mapping to registers.
 *  LABEL       des of branch and jump.
 *  BRANCH      if (lhs_) goto rhs_; // The 'if else' syntax is converted.
 *  JUMP        goto label.
 *  CALL        function call.
 */

class TAC {
public:
  bool IsConstant()   const { return t_ == CONSTANT; }
  bool IsVariable()   const { return t_ == VARIABLE; }
  bool IsTemporary()  const { return t_ == TEMPORARY; }
  bool IsLabel()      const { return t_ == LABEL; }
  bool IsBranch()     const { return t_ == BRANCH; }
  bool IsJump()       const { return t_ == JUMP; }
  bool IsCall()       const { return t_ == CALL; }

  bool IsInteger()    const { return dt_ == INTEGER; }
  bool IsFloat()      const { return dt_ == FLOAT; }
  bool IsAggregate()  const { return dt_ == AGGREGATE; }

private:
  enum {
    CONSTANT,
    VARIABLE,
    TEMPORARY,
    LABEL,
    BRANCH,
    JUMP,
    CALL,
  };

  enum {
    INTEGER,
    FLOAT,
    AGGREGATE,
  }

  TACType t_;

  // Data type: integer, float, aggregate
  uint8_t dt_: 2;
  uint8_t align_: 6;
  uint32_t width_;

  TAC* lhs_;
  TAC* rhs_;
};

#endif
