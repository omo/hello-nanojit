/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=4 sw=4 et tw=99:
 *
 * The author disclaims copyright to this source code.  In place of
 * a legal notice, here is a blessing:
 *
 *    May you do good and not evil.
 *    May you find forgiveness for yourself and forgive others.
 *    May you share freely, never taking more than you give.
 * 
 */
#include <nanojit/nanojit.h>

namespace nj = nanojit;
namespace ap = avmplus;

static GC g_gc = GC();
static nj::AvmCore g_core;

static void say_hello()
{
	puts("hello!\n");
}

nj::CallInfo builtins[] = {
	{ /* _address_ */ (intptr_t)&say_hello, 
	  /* _argtypes */ 0, 
	  /* _cse      */ (uint8_t)false, 
          /* _fold     */ (uint8_t)false, 
          /* _name     */ "say_hello" }
};

int main(int argc, char* argv[])
{
    nj::Fragmento* frago = new (&g_gc) nj::Fragmento(&g_core, 24);
    verbose_only(frago->labels = new (&g_gc) nj::LabelMap(&g_core, NULL););
    frago->assm()->setCallTable(builtins);

    nj::Fragment* f = frago->newLoop(0);
    f->root = f;
    f->calldepth = 0;
    f->lirbuf = new (&g_gc) nj::LirBuffer(frago, builtins); // deleted in Fragment::onDestroy()
    f->lirbuf->names = new (&g_gc) nj::LirNameMap(&g_gc, builtins, frago->labels); // deleted at Lirbuf::~LirBuffer()

    nj::LirWriter* lir = 0;
    nj::LirWriter* lir_buf_writer = lir = new (&g_gc) nj::LirBufWriter(f->lirbuf);
    nj::LirWriter* lir_cse_filter = lir = new (&g_gc) nj::CseFilter(lir, &g_gc);
    nj::LirWriter* lir_expr_filter = lir = new (&g_gc) nj::ExprFilter(lir);

    frago->assm()->setError(nj::None);

    // read function parameter
    lir->ins0(nj::LIR_trace);
    f->lirbuf->state = lir->insParam(0);
    f->lirbuf->param1 = lir->insParam(1);

    nj::LIns* args[] = {};
    lir->insCall(0 /* index on builtins array */, args);

    assert(!frago->assm()->error());

    nj::SideExit exit;
    memset(&exit, 0, sizeof(exit));
    exit.from = f;
    exit.calldepth = 0;
    exit.numGlobalSlots = 0;
    exit.numStackSlots = 0;
    exit.exitType = nj::LOOP_EXIT;
    exit.ip_adj = 0;
    exit.sp_adj = 0;
    exit.rp_adj = 0;

    ap::InterpState state;
    state.sp  = 0;
    state.eos = 0;
    state.rp  = 0;
    state.eor = 0;
    state.gp  = 0;
    state.cx  = 0;

    f->lastIns = lir->insGuard(nj::LIR_x, lir->insImm(1), &exit);
    compile(frago->assm(), f);

    union {
        nj::NIns *code; 
        nj::GuardRecord* (FASTCALL *func)(ap::InterpState*, nj::Fragment*); 
    } u;

    u.code = f->code();
    nj::GuardRecord* lr = u.func(&state, NULL);
    assert(lr->exit->exitType == nj::LOOP_EXIT); // as specified above

    delete lir_expr_filter;
    delete lir_cse_filter;
    delete lir_buf_writer;

    verbose_only(delete frago->labels;);
    delete frago;

    return 0;
}

