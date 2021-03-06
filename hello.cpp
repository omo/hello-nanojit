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

/*
 * function table, called inside JIT-ed code.
 * see LIR.h to definition, jsbuiltins.cpp for usage
 */
nj::CallInfo builtins[] = {
    { /* _address_ */ (intptr_t)&say_hello, 
      /* _argtypes */ 0,
      /* _cse      */ (uint8_t)false, 
      /* _fold     */ (uint8_t)false, 
      /* _name     */ "say_hello" }
};

int main(int argc, char* argv[])
{
    /*
     * create Fragmento, the JIT context object.
     * Fragmento is essentially a tree of Fragment objects.
     *
     * g_gc parameter to operator new is simply ignored. 
     * it was originally used Tamarin GC, which we don't not use.
     *
     * see jstracer.cpp:js_RecordTree() for realworld code
     */
    nj::Fragmento* frago = new (&g_gc) nj::Fragmento(&g_core, 24);
    verbose_only(frago->labels = new (&g_gc) nj::LabelMap(&g_core, NULL););
    frago->assm()->setCallTable(builtins);

    nj::Fragment* f = frago->newLoop(0);
    f->root = f;
    f->calldepth = 0;
    f->lirbuf = new (&g_gc) nj::LirBuffer(frago, builtins); // deleted in Fragment::onDestroy()
    f->lirbuf->names = new (&g_gc) nj::LirNameMap(&g_gc, builtins, frago->labels); // deleted at Lirbuf::~LirBuffer()

    /*
     * setup filter objects. filster objects is similar to Pass of LLVM in concept.
     * see jstracer.cpp:TraceRecorder::TraceRecorder()
     */
    nj::LirWriter* lir = 0;
    nj::LirWriter* lir_buf_writer = lir = new (&g_gc) nj::LirBufWriter(f->lirbuf);
    nj::LirWriter* lir_cse_filter = lir = new (&g_gc) nj::CseFilter(lir, &g_gc);
    nj::LirWriter* lir_expr_filter = lir = new (&g_gc) nj::ExprFilter(lir);

    /*
     * now we start tracing...
     */
    frago->assm()->setError(nj::None);
    lir->ins0(nj::LIR_trace); // pseudo instruction; just clear writer inernal state

    /* emit inst to read function parameter */
    f->lirbuf->state = lir->insParam(0);
    f->lirbuf->param1 = lir->insParam(1);

    /* call function: say_hello() takes no arg */
    nj::LIns* args[] = {};
    lir->insCall(0 /* index on builtins array */, args);

    assert(!frago->assm()->error());

    /*
     * we should exit (return from) the code at the end.
     *
     * SideExit object is returned from JIT-ed code to indicate
     * where it exited (guarded) at.
     * because the object is copied and embedded inside the code, you can dispose it freely.
     *
     * see TraceRecorder::snapshot(), TraceRecorder::guarded(), etc.
     */
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

    /* non-conditional branch(exit) */
    f->lastIns = lir->insGuard(nj::LIR_x, lir->insImm(1), &exit);

    /*
     * It's time to comile!
     *
     * TraceRecorder::closeLoop(), js_ContinueRecording()
     */
    compile(frago->assm(), f);

    /*
     * ... then invoke generated code.
     *
     * js_ExecuteTree()
     */
    union {
        nj::NIns *code; 
        nj::GuardRecord* (FASTCALL *func)(ap::InterpState*, nj::Fragment*); 
    } u;

    /*
     * InterpState is used to restore VM state inside native stack frame.
     * You should emit such restore routine yourself. 
     * In other word, we can just ignoreo InterpState parameter.
     */
    ap::InterpState state;
    state.sp  = 0;
    state.eos = 0;
    state.rp  = 0;
    state.eor = 0;
    state.gp  = 0;
    state.cx  = 0;

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

