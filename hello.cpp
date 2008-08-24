
#include <nanojit/nanojit.h>

namespace nj = nanojit;

static GC g_gc = GC();
static nj::AvmCore g_core;

int main(int argc, char* argv[])
{
  nj::Fragmento* frago = new (&g_gc) nj::Fragmento(&g_core, 24);
	verbose_only(frago->labels = new (&g_gc) nj::LabelMap(&g_core, NULL););
	verbose_only(delete frago->labels;);
  delete frago;
  return 0;
}

