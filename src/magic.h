// macro magic taken from http://saadahmad.ca/cc-preprocessor-metaprogramming-2/

#ifndef __MIXUP_MAGIC_H__
#define __MIXUP_MAGIC_H__

// Evaluating and Deferring Macro Calls
#define EMPTY()

#define DEFER(...) __VA_ARGS__ EMPTY()
#define DEFER2(...) __VA_ARGS__ DEFER(EMPTY)()
#define DEFER3(...) __VA_ARGS__ DEFER2(EMPTY)()
#define DEFER4(...) __VA_ARGS__ DEFER3(EMPTY)()
#define DEFER5(...) __VA_ARGS__ DEFER4(EMPTY)()

#define EVAL0_1(...) __VA_ARGS__
#define EVAL0_2(...) EVAL0_1(EVAL0_1(__VA_ARGS__))
#define EVAL0_3(...) EVAL0_2(EVAL0_2(__VA_ARGS__))
#define EVAL0_4(...) EVAL0_3(EVAL0_3(__VA_ARGS__))
#define EVAL0_5(...) EVAL0_4(EVAL0_4(__VA_ARGS__))
#define EVAL0_6(...) EVAL0_5(EVAL0_5(__VA_ARGS__))
#define EVAL0_7(...) EVAL0_6(EVAL0_6(__VA_ARGS__))
#define EVAL0_8(...) EVAL0_7(EVAL0_7(__VA_ARGS__))
#define EVAL0(...) EVAL0_8(__VA_ARGS__)

#define EVAL1_1(...) __VA_ARGS__
#define EVAL1_2(...) EVAL1_1(EVAL1_1(__VA_ARGS__))
#define EVAL1_3(...) EVAL1_2(EVAL1_2(__VA_ARGS__))
#define EVAL1_4(...) EVAL1_3(EVAL1_3(__VA_ARGS__))
#define EVAL1_5(...) EVAL1_4(EVAL1_4(__VA_ARGS__))
#define EVAL1_6(...) EVAL1_5(EVAL1_5(__VA_ARGS__))
#define EVAL1_7(...) EVAL1_6(EVAL1_6(__VA_ARGS__))
#define EVAL1_8(...) EVAL1_7(EVAL1_7(__VA_ARGS__))
#define EVAL1(...) EVAL1_8(__VA_ARGS__)

#define EVAL2_1(...) __VA_ARGS__
#define EVAL2_2(...) EVAL2_1(EVAL2_1(__VA_ARGS__))
#define EVAL2_3(...) EVAL2_2(EVAL2_2(__VA_ARGS__))
#define EVAL2_4(...) EVAL2_3(EVAL2_3(__VA_ARGS__))
#define EVAL2_5(...) EVAL2_4(EVAL2_4(__VA_ARGS__))
#define EVAL2_6(...) EVAL2_5(EVAL2_5(__VA_ARGS__))
#define EVAL2_7(...) EVAL2_6(EVAL2_6(__VA_ARGS__))
#define EVAL2_8(...) EVAL2_7(EVAL2_7(__VA_ARGS__))
#define EVAL2(...) EVAL2_8(__VA_ARGS__)

// Basic Pattern Matching
#define ENCLOSE_EXPAND(...) EXPANDED, ENCLOSED, (__VA_ARGS__) ) EAT (
#define GET_CAT_EXP(a, b) (a, ENCLOSE_EXPAND b, DEFAULT, b)

#define CAT_WITH_ENCLOSED(a, b) a b
#define CAT_WITH_DEFAULT(a, b) a##b
#define CAT_WITH(a, _, f, b) CAT_WITH_##f(a, b)

#define EVAL_CAT_WITH(...) CAT_WITH __VA_ARGS__
#define CAT(a, b) EVAL_CAT_WITH(GET_CAT_EXP(a, b))

#define IF_1(true, ...) true
#define IF_0(true, ...) __VA_ARGS__
#define IF(value) CAT(IF_, value)

#define EAT(...)
#define EXPAND_TEST_EXISTS(...) EXPANDED, EXISTS(__VA_ARGS__) ) EAT (
#define GET_TEST_EXISTS_RESULT(x) (CAT(EXPAND_TEST_, x), DOESNT_EXIST)

#define GET_TEST_EXIST_VALUE_(expansion, existValue) existValue
#define GET_TEST_EXIST_VALUE(x) GET_TEST_EXIST_VALUE_ x

#define TEST_EXISTS(x) GET_TEST_EXIST_VALUE(GET_TEST_EXISTS_RESULT(x))

#define DOES_VALUE_EXIST_EXISTS(...) 1
#define DOES_VALUE_EXIST_DOESNT_EXIST 0
#define DOES_VALUE_EXIST(x) CAT(DOES_VALUE_EXIST_, x)

#define EXTRACT_VALUE_EXISTS(...) __VA_ARGS__
#define EXTRACT_VALUE(value) CAT(EXTRACT_VALUE_, value)

#define TRY_EXTRACT_EXISTS(value, ...)                                         \
  IF(DOES_VALUE_EXIST(TEST_EXISTS(value)))(EXTRACT_VALUE(value), __VA_ARGS__)

#define AND_11 EXISTS(1)
#define AND(x, y) TRY_EXTRACT_EXISTS(CAT(CAT(AND_, x), y), 0)

#define OR_00 EXISTS(0)
#define OR(x, y) TRY_EXTRACT_EXISTS(CAT(CAT(OR_, x), y), 1)

#define XOR_01 EXISTS(1)
#define XOR_10 EXISTS(1)
#define XOR(x, y) TRY_EXTRACT_EXISTS(CAT(CAT(XOR_, x), y), 0)

#define NOT_0 EXISTS(1)
#define NOT(x) TRY_EXTRACT_EXISTS(CAT(NOT_, x), 0)

#define IS_ZERO_0 EXISTS(1)
#define IS_ZERO(x) TRY_EXTRACT_EXISTS(CAT(IS_ZERO_, x), 0)

#define IS_NOT_ZERO(x) NOT(IS_ZERO(x))

#define IS_ENCLOSED_TEST(...) EXISTS(1)
#define IS_ENCLOSED(x, ...) TRY_EXTRACT_EXISTS(IS_ENCLOSED_TEST x, 0)

// Lists and FOR_EACH
#define HEAD(x, ...) x
#define TAIL(x, ...) __VA_ARGS__

#define TEST_LAST EXISTS(1)
#define IS_LIST_EMPTY(...)                                                     \
  TRY_EXTRACT_EXISTS(DEFER(HEAD)(__VA_ARGS__ EXISTS(1)), 0)
#define IS_LIST_NOT_EMPTY(...) NOT(IS_LIST_EMPTY(__VA_ARGS__))

#define ENCLOSE(...) (__VA_ARGS__)
#define REM_ENCLOSE_(...) __VA_ARGS__
#define REM_ENCLOSE(...) REM_ENCLOSE_ __VA_ARGS__

#define IF_ENCLOSED_1(true, ...) true
#define IF_ENCLOSED_0(true, ...) __VA_ARGS__
#define IF_ENCLOSED(...) CAT(IF_ENCLOSED_, IS_ENCLOSED(__VA_ARGS__))
#define OPT_REM_ENCLOSE(...)                                                   \
  IF_ENCLOSED(__VA_ARGS__)(REM_ENCLOSE(__VA_ARGS__), __VA_ARGS__)

#define FOR_EACH_INDIRECT() FOR_EACH_NO_EVAL
#define FOR_EACH_NO_EVAL(T, ...)                                               \
  IF(IS_LIST_NOT_EMPTY(__VA_ARGS__))(DEFER(T)(OPT_REM_ENCLOSE(                 \
      HEAD(__VA_ARGS__))) DEFER2(FOR_EACH_INDIRECT)()(T, TAIL(__VA_ARGS__)))
#define FOR_EACH(stack, T, ...) EVAL##stack(FOR_EACH_NO_EVAL(T, __VA_ARGS__))

#define COUNT_(_0, _1, _2, _3, _4, _5, _6, _7, N, ...) N
#define COUNT(...) COUNT_(__VA_ARGS__ __VA_OPT__(, ), 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define FOR_EACH_IDX_INDIRECT() FOR_EACH_IDX_NO_EVAL
#define FOR_EACH_IDX_NO_EVAL(T, __VA_INDICES__, ...)                           \
  IF(IS_LIST_NOT_EMPTY(__VA_ARGS__))(                                          \
      DEFER(T)(OPT_REM_ENCLOSE(HEAD(__VA_ARGS__)),                             \
               DEFER(HEAD)(REM_ENCLOSE(__VA_INDICES__)))                       \
          DEFER2(FOR_EACH_IDX_INDIRECT)()(T, (TAIL __VA_INDICES__),            \
                                          TAIL(__VA_ARGS__)))
#define FOR_EACH_IDX(stack, T, ...)                                            \
  EVAL##stack(FOR_EACH_IDX_NO_EVAL(T, (0, 1, 2, 3, 4, 5, 6, 7), __VA_ARGS__))

#define FOR_EACH_MAP_INDIRECT() FOR_EACH_MAP_NO_EVAL
#define FOR_EACH_MAP_NO_EVAL(T, ...)                                           \
  IF(IS_LIST_NOT_EMPTY(__VA_ARGS__))(                                          \
      DEFER2(REM_ENCLOSE)((, DEFER(T)(OPT_REM_ENCLOSE(HEAD(__VA_ARGS__)))))    \
          DEFER2(FOR_EACH_MAP_INDIRECT)()(T, TAIL(__VA_ARGS__)))
#define FOR_EACH_MAP(stack, T, ...)                                            \
  IF(IS_LIST_NOT_EMPTY(__VA_ARGS__))(                                          \
      DEFER(T)(OPT_REM_ENCLOSE(HEAD(__VA_ARGS__))))                            \
      EVAL##stack(FOR_EACH_MAP_NO_EVAL(T, TAIL(__VA_ARGS__)))

#define UNIQUE(name) CAT(name##_, __LINE__)

#endif
