/*
** $Id: lstate.h,v 2.133 2016/12/22 13:08:50 roberto Exp $
** Global State
** See Copyright Notice in lua.h
*/

#ifndef lstate_h
#define lstate_h

#include "lua.h"

#include "lobject.h"
#include "ltm.h"
#include "lzio.h"


/*

** Some notes about garbage-collected objects: All objects in Lua must
** be kept somehow accessible until being freed, so all objects always
** belong to one (and only one) of these lists, using field 'next' of
** the 'CommonHeader' for the link:
**
** 'allgc': all objects not marked for finalization;
** 'finobj': all objects marked for finalization;
** 'tobefnz': all objects ready to be finalized;
** 'fixedgc': all objects that are not to be collected (currently
** only small strings, such as reserved words).


关于垃圾收集对象的一些注意事项:在释放之前，
Lua中的所有对象都必须以某种方式保持可访问性，
因此所有对象都属于这些列表中的一个(而且只有一个)，
使用“CommonHeader”的字段“next”作为链接:

**“allgc”:所有未标记为终结的对象;
** 'finobj':所有标记为终结的对象;
**“tobefnz”:所有准备完成的对象;
** * 'fixedgc':所有不被收集的对象(当前)
**只有小字符串，如保留字。

*/


struct lua_longjmp;  /* defined in ldo.c */


/*
** Atomic type (relative to signals) to better ensure that 'lua_sethook'
** is thread safe

	原子类型(相对于信号)，以更好地确保“lua_sethook”是线程安全的

*/
#if !defined(l_signalT)
#include <signal.h>
#define l_signalT	sig_atomic_t
#endif


/* extra stack space to handle TM calls and some other extras */
/* 处理TM呼叫和其他额外的栈空间 */
#define EXTRA_STACK   5


#define BASIC_STACK_SIZE        (2*LUA_MINSTACK)


/* kinds of Garbage Collection各种各样的垃圾收集*/
#define KGC_NORMAL	0
#define KGC_EMERGENCY	1	/* gc was forced by an allocation failure gc是由于分配失败而被迫的*/


typedef struct stringtable {
  TString **hash;
  int nuse;  /* number of elements */
  int size;
} stringtable;


/*
** Information about a call.
** When a thread yields, 'func' is adjusted to pretend that the
** top function has only the yielded values in its stack; in that
** case, the actual 'func' value is saved in field 'extra'.
** When a function calls another with a continuation, 'extra' keeps
** the function index so that, in case of errors, the continuation
** function can be called with the correct top.
调用信息。
当一个线程产生时，将调整“func”，
使其假设顶部函数在其堆栈中只有已生成的值;
在这种情况下，实际的“func”值保存在字段“extra”中。
当函数使用continuation调用另一个函数时，
'extra'保留函数索引，以便在出现错误时，
可以使用正确的顶部调用continuation函数。

*/
typedef struct CallInfo {
  StkId func;  /* function index in the stack函数在堆栈中的索引 */
  StkId	top;  /* top for this function 这个函数的顶*/
  struct CallInfo *previous, *next;  /* dynamic call link 动态调用链接*/
  union {
    struct {  /* only for Lua functions只对Lua函数 */
      StkId base;  /* base for this function 基地这个函数*/
      const Instruction *savedpc;
    } l;
    struct {  /* only for C functions只对C函数 */
      lua_KFunction k;  /* continuation in case of yields在产量情况下继续 */
      ptrdiff_t old_errfunc;
      lua_KContext ctx;  /* context info. in case of yields 上下文信息。的收益率*/
    } c;
  } u;
  ptrdiff_t extra;
  short nresults;  /* expected number of results from this function这个函数的期望结果数 */
  unsigned short callstatus;
} CallInfo;


/*
** Bits in CallInfo status   CallInfo状态位
*/
#define CIST_OAH	(1<<0)	/* original value of 'allowhook' “allowhook”的原始值*/
#define CIST_LUA	(1<<1)	/* call is running a Lua function调用运行Lua函数。*/
#define CIST_HOOKED	(1<<2)	/* call is running a debug hook 调用正在运行一个调试挂钩*/
#define CIST_FRESH	(1<<3)	/* call is running on a fresh invocation
                                   of luaV_execute 调用在luaV_execute的新调用上运行 */
#define CIST_YPCALL	(1<<4)	/* call is a yieldable protected call可屈服的受保护的 */
#define CIST_TAIL	(1<<5)	/* call was tail called尾部 调用 */
#define CIST_HOOKYIELD	(1<<6)	/* last hook called yielded最后一个钩子被调用 */
#define CIST_LEQ	(1<<7)  /* using __lt for __le */
#define CIST_FIN	(1<<8)  /* call is running a finalizer调用运行终结器 */

#define isLua(ci)	((ci)->callstatus & CIST_LUA)

/* assume that CIST_OAH has offset 0 and that 'v' is strictly 0/1
假设Cistyooa偏移了0，而“v”是严格的0/1*/
#define setoah(st,v)	((st) = ((st) & ~CIST_OAH) | (v))
#define getoah(st)	((st) & CIST_OAH)


/*
** 'global state', shared by all threads of this state
“全局状态”，由该状态的所有线程共享

*/
typedef struct global_State {
  lua_Alloc frealloc;  /* function to reallocate memory重新分配内存的功能 */
  void *ud;         /* auxiliary data to 'frealloc'  “FaloLc”的辅助数据*/
  l_mem totalbytes;  /* number of bytes currently allocated - GCdebt 当前分配的字节数- GCdebt*/
  l_mem GCdebt;  /* bytes allocated not yet compensated by the collector 尚未被收集器补偿的字节分配*/
  lu_mem GCmemtrav;  /* memory traversed by the GC GC遍历存储器*/
  lu_mem GCestimate;  /* an estimate of the non-garbage memory in use 使用中的非垃圾存储器的估计*/
  stringtable strt;  /* hash table for strings字符串哈希表 */
  TValue l_registry;
  unsigned int seed;  /* randomized seed for hashes哈希表随机种子 */
  lu_byte currentwhite;
  lu_byte gcstate;  /* state of garbage collector垃圾收集器状态 */
  lu_byte gckind;  /* kind of GC running GC运行方式*/
  lu_byte gcrunning;  /* true if GC is running GC正在运行为真*/
  GCObject *allgc;  /* list of all collectable objects所有可收集对象列表 */
  GCObject **sweepgc;  /* current position of sweep in list 列表中当前的扫描位置*/
  GCObject *finobj;  /* list of collectable objects with finalizers带终结器的可收集对象列表 */
  GCObject *gray;  /* list of gray objects灰度对象列表 */
  GCObject *grayagain;  /* list of objects to be traversed atomically以原子方式遍历的对象的列表 */
  GCObject *weak;  /* list of tables with weak values具有弱值的表的列表。 */
  GCObject *ephemeron;  /* list of ephemeron tables (weak keys) 星历表列表（弱键）*/
  GCObject *allweak;  /* list of all-weak tables 一切弱小的表列表*/
  GCObject *tobefnz;  /* list of userdata to be GC  GC用户数据列表 */
  GCObject *fixedgc;  /* list of objects not to be collected 不收集对象列表*/
  struct lua_State *twups;  /* list of threads with open upvalues具有开放值的线程列表 */
  unsigned int gcfinnum;  /* number of finalizers to call in each GC step在每个GC步骤中调用的终结器的数目 */
  int gcpause;  /* size of pause between successive GCs 连续GCS间的停顿尺寸*/
  int gcstepmul;  /* GC 'granularity'“粒度” */
  lua_CFunction panic;  /* to be called in unprotected errors 在不受保护的错误中被调用*/
  struct lua_State *mainthread;
  const lua_Number *version;  /* pointer to version number 指针版本号*/
  TString *memerrmsg;  /* memory-error message 内存错误消息 */
  TString *tmname[TM_N];  /* array with tag-method names带标记方法名称的数组 */
  struct Table *mt[LUA_NUMTAGS];  /* metatables for basic types基本类型的元表 */
  TString *strcache[STRCACHE_N][STRCACHE_M];  /* cache for strings in API在API中缓存字符串 */
} global_State;


/*
** 'per thread' state每个线程的状态
*/
struct lua_State {
  CommonHeader;
  unsigned short nci;  /* number of items in 'ci' list “ci”列表中的项目数量*/
  lu_byte status;
  StkId top;  /* first free slot in the stack 栈中第一个空闲插槽*/
  global_State *l_G;
  CallInfo *ci;  /* call info for current function调用当前函数的信息 */
  const Instruction *oldpc;  /* last pc traced */
  StkId stack_last;  /* last free slot in the stack栈中最后一个空闲插槽 */
  StkId stack;  /* stack base栈基 */
  UpVal *openupval;  /* list of open upvalues in this stack 此堆栈中打开的向上值的列表*/
  GCObject *gclist;
  struct lua_State *twups;  /* list of threads with open upvalues具有打开向上值的线程的列表 */
  struct lua_longjmp *errorJmp;  /* current error recover point 当前误差恢复点*/
  CallInfo base_ci;  /* CallInfo for first level (C calling Lua) 第一级CallInfo (C调用Lua)*/
  volatile lua_Hook hook;
  ptrdiff_t errfunc;  /* current error handling function (stack index)当前错误处理函数(堆栈索引) */
  int stacksize;
  int basehookcount;
  int hookcount;
  unsigned short nny;  /* number of non-yieldable calls in stack 栈中不可让步调用的数量 */
  unsigned short nCcalls;  /* number of nested C calls嵌套C调用的数量 */
  l_signalT hookmask;
  lu_byte allowhook;
};


#define G(L)	(L->l_G)


/*
** Union of all collectable objects (only for conversions)
所有可收藏对象的联合(仅用于转换)

*/
union GCUnion {
  GCObject gc;  /* common header */
  struct TString ts;
  struct Udata u;
  union Closure cl;
  struct Table h;
  struct Proto p;
  struct lua_State th;  /* thread */
};


#define cast_u(o)	cast(union GCUnion *, (o))

/* macros to convert a GCObject into a specific value将GCObject转换为特定值的宏 */
#define gco2ts(o)  \
	check_exp(novariant((o)->tt) == LUA_TSTRING, &((cast_u(o))->ts))
#define gco2u(o)  check_exp((o)->tt == LUA_TUSERDATA, &((cast_u(o))->u))
#define gco2lcl(o)  check_exp((o)->tt == LUA_TLCL, &((cast_u(o))->cl.l))
#define gco2ccl(o)  check_exp((o)->tt == LUA_TCCL, &((cast_u(o))->cl.c))
#define gco2cl(o)  \
	check_exp(novariant((o)->tt) == LUA_TFUNCTION, &((cast_u(o))->cl))
#define gco2t(o)  check_exp((o)->tt == LUA_TTABLE, &((cast_u(o))->h))
#define gco2p(o)  check_exp((o)->tt == LUA_TPROTO, &((cast_u(o))->p))
#define gco2th(o)  check_exp((o)->tt == LUA_TTHREAD, &((cast_u(o))->th))


/* macro to convert a Lua object into a GCObject
将Lua对象转换为GCObject的宏 */
#define obj2gco(v) \
	check_exp(novariant((v)->tt) < LUA_TDEADKEY, (&(cast_u(v)->gc)))


/* actual number of total bytes allocated
分配的总字节数 */
#define gettotalbytes(g)	cast(lu_mem, (g)->totalbytes + (g)->GCdebt)

LUAI_FUNC void luaE_setdebt (global_State *g, l_mem debt);
LUAI_FUNC void luaE_freethread (lua_State *L, lua_State *L1);
LUAI_FUNC CallInfo *luaE_extendCI (lua_State *L);
LUAI_FUNC void luaE_freeCI (lua_State *L);
LUAI_FUNC void luaE_shrinkCI (lua_State *L);


#endif

