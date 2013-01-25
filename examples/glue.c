#include "glue.h"
#include "../src/internal.h"  // for h_carray_*


// The action equivalent of h_ignore.
const HParsedToken *h_act_ignore(const HParseResult *p)
{
  return NULL;
}

// Helper to build HAction's that pick one index out of a sequence.
const HParsedToken *h_act_index(int i, const HParseResult *p)
{
    if(!p) return NULL;

    const HParsedToken *tok = p->ast;

    if(!tok || tok->token_type != TT_SEQUENCE)
        return NULL;

    const HCountedArray *seq = tok->seq;
    size_t n = seq->used;

    if(i<0 || (size_t)i>=n)
        return NULL;
    else
        return tok->seq->elements[i];
}

// Action version of h_seq_flatten.
const HParsedToken *h_act_flatten(const HParseResult *p) {
  return h_seq_flatten(p->arena, p->ast);
}

// Low-level helper for the h_make family.
HParsedToken *h_make_(HArena *arena, HTokenType type)
{
  HParsedToken *ret = h_arena_malloc(arena, sizeof(HParsedToken));
  ret->token_type = type;
  return ret;
}

HParsedToken *h_make(HArena *arena, HTokenType type, void *value)
{
  assert(type >= TT_USER);
  HParsedToken *ret = h_make_(arena, type);
  ret->user = value;
  return ret;
}

HParsedToken *h_make_seq(HArena *arena)
{
  HParsedToken *ret = h_make_(arena, TT_SEQUENCE);
  ret->seq = h_carray_new(arena);
  return ret;
}

HParsedToken *h_make_bytes(HArena *arena, size_t len)
{
  HParsedToken *ret = h_make_(arena, TT_BYTES);
  ret->bytes.len = len;
  ret->bytes.token = h_arena_malloc(arena, len);
  return ret;
}

HParsedToken *h_make_sint(HArena *arena, int64_t val)
{
  HParsedToken *ret = h_make_(arena, TT_SINT);
  ret->sint = val;
  return ret;
}

HParsedToken *h_make_uint(HArena *arena, uint64_t val)
{
  HParsedToken *ret = h_make_(arena, TT_UINT);
  ret->uint = val;
  return ret;
}

void * h_cast(HTokenType type, const HParsedToken *p)
{
  assert(p->token_type == type);
  return p->user;
}

HCountedArray *h_cast_seq(const HParsedToken *p)
{
  assert(p->token_type == TT_SEQUENCE);
  return p->seq;
}

HBytes h_cast_bytes(const HParsedToken *p)
{
  assert(p->token_type == TT_BYTES);
  return p->bytes;
}

int64_t h_cast_sint(const HParsedToken *p)
{
  assert(p->token_type == TT_SINT);
  return p->sint;
}

uint64_t h_cast_uint(const HParsedToken *p)
{
  assert(p->token_type == TT_UINT);
  return p->uint;
}

// XXX -> internal
HParsedToken *h_carray_index(const HCountedArray *a, size_t i)
{
  assert(i < a->used);
  return a->elements[i];
}

size_t h_seq_len(const HParsedToken *p)
{
  assert(p != NULL);
  assert(p->token_type == TT_SEQUENCE);
  return p->seq->used;
}

HParsedToken **h_seq_elements(const HParsedToken *p)
{
  assert(p != NULL);
  assert(p->token_type == TT_SEQUENCE);
  return p->seq->elements;
}

HParsedToken *h_seq_index(const HParsedToken *p, size_t i)
{
  assert(p != NULL);
  assert(p->token_type == TT_SEQUENCE);
  return h_carray_index(p->seq, i);
}

HCountedArray *h_seq_index_seq(const HParsedToken *p, size_t i)
{
  return h_cast_seq(h_seq_index(p, i));
}

HBytes h_seq_index_bytes(const HParsedToken *p, size_t i)
{
  return h_cast_bytes(h_seq_index(p, i));
}

int64_t h_seq_index_sint(const HParsedToken *p, size_t i)
{
  return h_cast_sint(h_seq_index(p, i));
}

uint64_t h_seq_index_uint(const HParsedToken *p, size_t i)
{
  return h_cast_uint(h_seq_index(p, i));
}

void *h_seq_index_user(HTokenType type, const HParsedToken *p, size_t i)
{
  return h_cast(type, h_seq_index(p, i));
}

void h_seq_snoc(HParsedToken *xs, const HParsedToken *x)
{
  assert(xs != NULL);
  assert(xs->token_type == TT_SEQUENCE);

  h_carray_append(xs->seq, (HParsedToken *)x);
}

void h_seq_append(HParsedToken *xs, const HParsedToken *ys)
{
  assert(xs != NULL);
  assert(xs->token_type == TT_SEQUENCE);
  assert(ys != NULL);
  assert(ys->token_type == TT_SEQUENCE);

  for(size_t i; i<ys->seq->used; i++)
    h_carray_append(xs->seq, ys->seq->elements[i]);
}

// Flatten nested sequences. Always returns a sequence.
// If input element is not a sequence, returns it as a singleton sequence.
const HParsedToken *h_seq_flatten(HArena *arena, const HParsedToken *p)
{
  assert(p != NULL);

  HParsedToken *ret = h_make_seq(arena);
  switch(p->token_type) {
  case TT_SEQUENCE:
    // Flatten and append all.
    for(size_t i; i<p->seq->used; i++) {
      h_seq_append(ret, h_seq_flatten(arena, h_seq_index(p, i)));
    }
    break;
  default:
    // Make singleton sequence.
    h_seq_snoc(ret, p);
    break;
  }

  return ret;
}
