#ifndef PASS_H
#define PASS_H

#include <platform.h>
#include "../ast/ast.h"
#include "../ast/frame.h"
#include "../ast/source.h"


PONY_EXTERN_C_BEGIN


/** Passes

The passes are split into 3 groups:

1. Module passes.

Run on every Pony source file when it is loaded, regardless of what pass any
other part of the AST is in.
Not run on ASTs built directly via the BUILD macros.

2. AST passes.

Run on all ASTs whether made from source files or generated by the compiler.

In general the sugar pass is run for each file after the module passes and the
other passes are then run for the whole program together. However, when a new
sub-AST is created, eg by sugar, we go back and apply passes to that sub-AST
until it has caught up with the rest of the AST. It is then treated as part of
the overall AST for the remaining passes.

Note that this process of going back and catching up passes for a sub-AST may
happen recursively. For example, during the expr pass a new anonymous class $1
may be created. We go back to catch that up and in it's sugar pass it creates
another new anonymous class $2. Now we catch up $2 to the main tree (expr pass)
and then finish catching up $1.

3. Generate passes.

Run on the whole program AST at once.


* Parse pass (module)

Turns a source file in an AST. Deliberately allows some illegal syntax to
enable better error reporting.

Expects Pony source file or string.

Various AST flags are used to pass information to the syntax pass.


* Syntax pass (module)

Checks for specific illegal syntax cases that the BNF allows. This allows for
better error reporting. If this pass succeeds then the AST is fully
syntactically correct.

Does not change the AST.


* Sugar pass (AST)

Expands the AST to put in the code we've let the programmer miss out. This
includes default capabilities, method return values and else blocks. There are
also some code rewrites, such as assignment to update call and for loop to
while loop.

Not all sugar is perform here since some requires type check information which
is not yet available.

Substantially rewrites the AST.


* Scope pass (AST)

Creates entries in the relevant symbol tables for types, fields, methods,
locals, etc including initialising symbol definition status.

Also handles use commands, including loading other packages. Aliased used
commands are fully handled immediately. For use commands without aliases, the
used package is loaded but the resulting definitions are not merged into the
using package until the import pass.

Other than use commands, does not change the AST itself, only its symbol table
entries.


* Import pass (AST)

Imports symbols from packages "used" without an alias. This can't be done in
the scope pass due to complications from handling circular dependencies between
packages.

Does not change the AST itself, only module symbol table entries.


* Name resolution pass (AST)

TODO


* Flatten pass (AST)

TODO


* Traits pass (AST)

Adds methods inherited by entities from traits and interfaces, including
handling default bodies. Only changes AST by adding methods to types and bodies
to previously bodiless methods.

Uses the data field of various AST nodes:
* For entities, as a marker to spot dependency loops.
* For methods, internally for storing a data block. At the end of pass points
  to the entity that provided the method body used, NULL if method has no body.
* For method bodies and error indicators, points to an entity providing an
  ambiguous default body.

Also performs minor anomalous tasks that have to be done before the type check
pass.


* Documentation generation pass (AST)

Auto-generates documentation, if the relevant command line flag is given,
otherwise does nothing.

Does not alter the AST at all.


* Reference resolution pass (AST)

Resolves all instances of TK_REFERENCE by finding the relevant definition in
the symbol table and converting to an appropriate node type.

Attaches a link to the definition (as ast_data) for all resolved nodes.

Tracks symbol status for every reference through the AST to ensure that
definitions and consumes of references are respected across time/space.

Mutates the AST extensively, including setting AST_FLAG_JUMPS_AWAY and
AST_FLAG_INCOMPLETE flags on AST nodes, as appropriate.


* Expression type check pass (AST)

Resolves types for all expressions and confirms type safety of the program.
Also performs some "sugar" replacements that require knowledge of types.

Mutates the AST extensively.


* Verify pass (AST)

Perform various checks that are not required for type resolution, and are not
intrinsically related to the work done in the expression type check pass.
These checks may or may not require types to be resolved.

Does not mutate the structure of the AST, but may set and use flags.


* Finaliser pass (AST)

Checks that any finalisers do not perform any restricted operations, such as
creating actors or sending messages.

Within finalisers uses the data field of the top body node and any TK_CALL
nodes as ast_send flags.

ADD INFORMATION ABOUT DESCHEDULER PASS

* Adding a new pass

It's usually best to start with an existing pass, look through the contents
of src/libponyc/pass and choose a pass closest to what you need.

Next edit pass.h add to enum pass_id an id for you pass, like mypass_id
and also add a line to PASS_HELP for instance, "    =mypass\n".

The final change is to edit pass.c. Include "mypass.h" in the list of other
includes. Add a line to return "mypass" in pass_name() and last but not least
add something like the following to ast_passes(), so your pass is invoked:

  if(!visit_pass(astp, options, last, &r, PASS_MYPASS, pre_pass_mypass,
    pass_mypass))
    return r;

You should now be able to use make to recompile the compiler. If you execute
"./build/release/ponyc --help" you should see mypass in the list of passes.

You may also want to add to the comments above documenting what your new
pass does.
*/

typedef enum verbosity_level
{
  VERBOSITY_QUIET     = 0,
  VERBOSITY_MINIMAL   = 1,
  VERBOSITY_INFO      = 2,
  VERBOSITY_TOOL_INFO = 3,
  VERBOSITY_ALL       = 4
} verbosity_level;

/** When updating pass_id update PASS_HELP
 */
typedef enum pass_id
{
  PASS_PARSE,
  PASS_SYNTAX,
  PASS_SUGAR,
  PASS_SCOPE,
  PASS_IMPORT,
  PASS_NAME_RESOLUTION,
  PASS_FLATTEN,
  PASS_TRAITS,
  PASS_DOCS,
  PASS_REFER,
  PASS_EXPR,
  PASS_VERIFY,
  PASS_FINALISER,
  // PASS_DESCHEDULER
  PASS_SERIALISER,
  PASS_REACH,
  PASS_PAINT,
  PASS_LLVM_IR,
  PASS_BITCODE,
  PASS_ASM,
  PASS_OBJ,
  PASS_ALL
} pass_id;

/** Update PASS_HELP when pass_id changes
 */
#define PASS_HELP \
    "  --pass, -r       Restrict phases.\n" \
    "    =parse\n" \
    "    =syntax\n" \
    "    =sugar\n" \
    "    =scope\n" \
    "    =import\n" \
    "    =name\n" \
    "    =flatten\n" \
    "    =traits\n" \
    "    =docs\n" \
    "    =refer\n" \
    "    =expr\n" \
    "    =verify\n" \
    "    =final\n" \
    "    =serialise\n" \
    "    =reach\n" \
    "    =paint\n" \
    "    =ir            Output LLVM IR.\n" \
    "    =bitcode       Output LLVM bitcode.\n" \
    "    =asm           Output assembly.\n" \
    "    =obj           Output an object file.\n" \
    "    =all           The default: generate an executable.\n"

typedef struct magic_package_t magic_package_t;
typedef struct plugins_t plugins_t;

/** Pass options.
 */
typedef struct pass_opt_t
{
  pass_id limit;
  pass_id program_pass;
  bool release;
  bool library;
  bool runtimebc;
  bool staticbin;
  bool pic;
  bool print_stats;
  bool verify;
  bool extfun;
  bool simple_builtin;
  bool strip_debug;
  bool print_filenames;
  bool check_tree;
  bool lint_llvm;
  bool docs;
  bool docs_private;

  verbosity_level verbosity;

  size_t ast_print_width;
  bool allow_test_symbols;
  bool parse_trace;

  strlist_t* package_search_paths;
  strlist_t* safe_packages;
  magic_package_t* magic_packages;

  const char* argv0;
  const char* output;
  const char* bin_name;
  char* link_arch;
  char* linker;

  char* triple;
  char* cpu;
  char* features;

  typecheck_t check;

  plugins_t* plugins;

  void* data; // User-defined data for unit test callbacks.
} pass_opt_t;

/** Limit processing to the specified pass. All passes up to and including the
 * specified pass will occur.
 * Returns true on success, false on invalid pass name.
 */
bool limit_passes(pass_opt_t* opt, const char* pass);

/** Report the name of the specified pass.
 * The returned string is a literal and should not be freed.
 */
const char* pass_name(pass_id pass);

/** Report the pass after the specified one.
 */
pass_id pass_next(pass_id pass);

/** Report the pass before the specified one.
*/
pass_id pass_prev(pass_id pass);

/** Initialise pass options.
 */
void pass_opt_init(pass_opt_t* options);

/** Finish with pass options.
 */
void pass_opt_done(pass_opt_t* options);

/** Apply the per module passes to the given source.
 * Returns true on success, false on failure.
 * The given source is attached to the resulting AST on success and closed on
 * failure.
 */
bool module_passes(ast_t* package, pass_opt_t* options, source_t* source);

/** Apply the AST passes to the given whole program AST.
 * Returns true on success, false on failure.
 */
bool ast_passes_program(ast_t* program, pass_opt_t* options);

/** Catch up the given newly created type definition sub-AST to whichever pass
 * its containing package has reached.
 * Returns true on success, false on failure.
 *
 * The current pass that types should be caught up to is stored in pass_opt_t.
 * Due to the above assumptions we catch up type sub-ASTs by applying all
 * passes BEFORE the stored value, not including it.
 *
 * A fail should be treated as an AST_FATAL, since some of the AST may not have
 * been through some passes and so may not be in a state that the current pass
 * expects.
 */
bool ast_passes_type(ast_t** astp, pass_opt_t* options, pass_id last_pass);

/** Catch up the given sub-AST to the specified pass.
 * Returns true on success, false on failure.
 *
 * If the previous pass needs to be specified it is recommended to use
 * pass_prev() rather than hardcoding, as this will protect against any future
 * changes in the pass order.
 *
 * A fail should be treated as an AST_FATAL, since some of the AST may not have
 * been through some passes and so may not be in a state that the current pass
 * expects.
 */
bool ast_passes_subtree(ast_t** astp, pass_opt_t* options, pass_id last_pass);

/** Perform the code generation passes based on the given AST.
 * Returns true on success, false on failure.
 */
bool generate_passes(ast_t* program, pass_opt_t* options);

/** Record the specified pass as done for the given AST.
 */
void ast_pass_record(ast_t* ast, pass_id pass);


typedef ast_result_t(*ast_visit_t)(ast_t** astp, pass_opt_t* options);

/** Perform the specified pass on the given AST.
 * The specified pass is stored in the AST and passes will not be repeated.
 * To suppress this check, and execute the given pass regardless, specify the
 * pass as PASS_ALL. No pass will be recorded in the AST in this case.
 */
ast_result_t ast_visit(ast_t** ast, ast_visit_t pre, ast_visit_t post,
  pass_opt_t* options, pass_id pass);

ast_result_t ast_visit_scope(ast_t** ast, ast_visit_t pre, ast_visit_t post,
  pass_opt_t* options, pass_id pass);


PONY_EXTERN_C_END

#endif
