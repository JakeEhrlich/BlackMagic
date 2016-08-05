/******************************************************************************************[Main.C]
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#define NDEBUG

#include <ctime>
#include <cstring>
#include <stdint.h>
#include <errno.h>

#include <signal.h>

#include "Solver.h"

/*************************************************************************************/
#ifdef _MSC_VER
#include <ctime>

static inline double cpuTime(void) {
    return (double)clock() / CLOCKS_PER_SEC; }
#else

#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

static inline double cpuTime(void) {
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return (double)ru.ru_utime.tv_sec + (double)ru.ru_utime.tv_usec / 1000000; }
#endif


#if defined(__linux__)
static inline int memReadStat(int field)
{
    char    name[256];
    pid_t pid = getpid();
    sprintf(name, "/proc/%d/statm", pid);
    FILE*   in = fopen(name, "rb");
    if (in == NULL) return 0;
    int     value;
    for (; field >= 0; field--)
        fscanf(in, "%d", &value);
    fclose(in);
    return value;
}
static inline uint64_t memUsed() { return (uint64_t)memReadStat(0) * (uint64_t)getpagesize(); }


#elif defined(__FreeBSD__)
static inline uint64_t memUsed(void) {
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return ru.ru_maxrss*1024; }


#else
static inline uint64_t memUsed() { return 0; }
#endif

#if defined(__linux__)
#include <fpu_control.h>
#endif

//=================================================================================================
// DIMACS Parser:

#define CHUNK_LIMIT 1048576

class StreamBuffer {
    FILE*  in;
    char    buf[CHUNK_LIMIT];
    int     pos;
    int     size;

    void assureLookahead() {
        if (pos >= size) {
            pos  = 0;
            size = fread(buf, 1, sizeof(buf), in); } }

public:
    StreamBuffer(FILE* i) : in(i), pos(0), size(0) {
        assureLookahead(); }

    int  operator *  () { return (pos >= size) ? EOF : buf[pos]; }
    void operator ++ () { pos++; assureLookahead(); }
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template<class B>
static void skipWhitespace(B& in) {
    while ((*in >= 9 && *in <= 13) || *in == 32)
        ++in; }

template<class B>
static void skipLine(B& in) {
    for (;;){
        if (*in == EOF || *in == '\0') return;
        if (*in == '\n') { ++in; return; }
        ++in; } }

template<class B>
static int parseInt(B& in) {
    int     val = 0;
    bool    neg = false;
    skipWhitespace(in);
    if      (*in == '-') neg = true, ++in;
    else if (*in == '+') ++in;
    if (*in < '0' || *in > '9') reportf("PARSE ERROR! Unexpected char: %c\n", *in), exit(3);
    while (*in >= '0' && *in <= '9')
        val = val*10 + (*in - '0'),
        ++in;
    return neg ? -val : val; }

template<class B>
static void readClause(B& in, Solver& S, vec<Lit>& lits) {
    int     parsed_lit, var;
    lits.clear();
    for (;;){
        parsed_lit = parseInt(in);
        if (parsed_lit == 0) break;
        var = abs(parsed_lit)-1;
        while (var >= S.nVars()) S.newVar();
        lits.push( (parsed_lit > 0) ? Lit(var) : ~Lit(var) );
    }
}

template<class B>
static bool match(B& in, const char* str) {
    for (; *str != 0; ++str, ++in)
        if (*str != *in)
            return false;
    return true;
}


template<class B>
static void parse_DIMACS_main(B& in, Solver& S) {
    vec<Lit> lits;
    for (;;){
        skipWhitespace(in);
        if (*in == EOF || *in =='\0')
            break;
        else if (*in == 'p'){
            if (match(in, "p cnf")){
                int vars    = parseInt(in);
                int clauses = parseInt(in);
            }else{
                reportf("PARSE ERROR! Unexpected char: %c\n", *in), exit(3);
            }
        } else if (*in == 'c' || *in == 'p')
            skipLine(in);
        else
            readClause(in, S, lits),
            S.addClause(lits);
    }
}

// Inserts problem into solver.
//
static void parse_DIMACS(FILE* input_stream, Solver& S) {
    StreamBuffer in(input_stream);
    parse_DIMACS_main(in, S);
}

static void parse_DIMACS(const char* cnfdata, Solver& S) {
    parse_DIMACS_main(cnfdata, S);
}

extern "C" {
    void* newSolverState(const char* cnfdata) {
        Solver* s = new Solver();
        s->verbosity = 0;
        parse_DIMACS(cnfdata, *s);
        if(!s->simplify()) {
            delete s;
            return nullptr;
        }
        //puts("about to return solver");
        return s;
    }

    void freeSolverState(void* state) {
        //puts("about to free solver");
        delete reinterpret_cast<Solver*>(state);
    }

    void solverStateAddClause(void* state, const int* clause) {
        Solver* s = reinterpret_cast<Solver*>(state);
        vec<Lit> lits;
        //puts("constructing clause");
        while(*clause) {
             int var = abs(*clause) - 1;
             lits.push( (*clause > 0) ? Lit(var) : ~Lit(var));
             ++clause;
        }
        //puts("adding constructed clause");
        s->addClause(lits);
    }

    int solverStateSolve(void* state, int* out) {
        Solver* s = reinterpret_cast<Solver*>(state);
        //puts("solving...");
        bool ret = s->solve();
        //puts("solved");
        if(ret) {
            //puts("found solution");
            for(int i = 0; i < s->nVars(); ++i) {
                if(s->model[i] == l_Undef) out[i] = 0;
                else if(s->model[i] == l_True) out[i] = 1;
                else out[i] = -1;
            }
            out[s->nVars()] = 0;
            //puts("solution output");
        } //else puts("no solution");
        //puts("returning solution");
        return ret;
    }
    void solveTest(const char* msg, int times) {
        while(times--) puts(msg);
    }
}

int main(int argc, char** argv)
{
    puts("hello, world!");    
}

