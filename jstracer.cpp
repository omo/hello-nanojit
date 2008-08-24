/*
 * copied from jstracer.cpp
 */

#include <nanojit/nanojit.h>

#define JS_ASSERT NanoAssert
using namespace nanojit;

class TreeInfo MMGC_SUBCLASS_DECL {
    nanojit::Fragment*      fragment;
public:
#if 0
    JSScript*               script;
    unsigned                maxNativeStackSlots;
    ptrdiff_t               nativeStackBase;
    unsigned                maxCallDepth;
    TypeMap                 stackTypeMap;
    unsigned                mismatchCount;
#endif    
    TreeInfo(nanojit::Fragment* _fragment) { 
        fragment = _fragment; 
    }
};

struct FrameInfo {
#if 0
    JSObject*       callee;     // callee function object
    jsbytecode*     callpc;     // pc of JSOP_CALL in caller script
    union {
        struct {
            uint16  spdist;     // distance from fp->slots to fp->regs->sp at JSOP_CALL
            uint16  argc;       // actual argument count, may be < fun->nargs
        } s;
        uint32      word;       // for spdist/argc LIR store in record_JSOP_CALL
    };
#else
	int dummy;
#endif
};

int
nanojit::StackFilter::getTop(LInsp guard)
{
    if (sp == frag->lirbuf->sp)
        return guard->exit()->sp_adj + sizeof(double);
    JS_ASSERT(sp == frag->lirbuf->rp);
    return guard->exit()->rp_adj + sizeof(FrameInfo);
}

#if defined NJ_VERBOSE
void
nanojit::LirNameMap::formatGuard(LIns *i, char *out)
{
    uint32_t ip;
    SideExit *x;

    x = (SideExit *)i->exit();
    ip = intptr_t(x->from->ip) + x->ip_adj;
    sprintf(out,
#if 0
        "%s: %s %s -> %s sp%+ld rp%+ld",
#else
        "%s: %s %s -> %s sp%+d rp%+d",
#endif
        formatRef(i),
        lirNames[i->opcode()],
        i->oprnd1()->isCond() ? formatRef(i->oprnd1()) : "",
        labels->format((void *)ip),
        x->sp_adj,
        x->rp_adj
        );
}
#endif

void
nanojit::Assembler::initGuardRecord(LIns *guard, GuardRecord *rec)
{
    SideExit *exit;

    exit = guard->exit();
    rec->guard = guard;
    rec->calldepth = exit->calldepth;
    rec->exit = exit;
    verbose_only(rec->sid = exit->sid);
}

void
nanojit::Assembler::asm_bailout(LIns *guard, Register state)
{
    /* we adjust ip/sp/rp when exiting from the tree in the recovery code */
}

void
nanojit::Fragment::onDestroy()
{
    if (root == this) {
        delete mergeCounts;
        delete lirbuf;
    }
    delete (TreeInfo *)vmprivate;
}

