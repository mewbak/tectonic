#include <tectonic/tectonic.h>
#include <tectonic/internals.h>
#include <tectonic/xetexd.h>
#include <tectonic/synctex.h>
#include <tectonic/stubs.h>
#include <setjmp.h>

/* Read and write dump files.  As distributed, these files are
   architecture dependent; specifically, BigEndian and LittleEndian
   architectures produce different files.  These routines always output
   BigEndian files.  This still does not guarantee them to be
   architecture-independent, because it is possible to make a format
   that dumps a glue ratio, i.e., a floating-point number.  Fortunately,
   none of the standard formats do that.  */

#if !defined (WORDS_BIGENDIAN)

/* This macro is always invoked as a statement.  It assumes a variable
   `temp'.  */

#define SWAP(x, y) temp = (x); (x) = (y); (y) = temp


/* Make the NITEMS items pointed at by P, each of size SIZE, be the
   opposite-endianness of whatever they are now.  */

static void
swap_items (char *p, int nitems, int size)
{
  char temp;

  /* Since `size' does not change, we can write a while loop for each
     case, and avoid testing `size' for each time.  */
  switch (size)
    {
    /* 16-byte items happen on the DEC Alpha machine when we are not
       doing sharable memory dumps.  */
    case 16:
      while (nitems--)
        {
          SWAP (p[0], p[15]);
          SWAP (p[1], p[14]);
          SWAP (p[2], p[13]);
          SWAP (p[3], p[12]);
          SWAP (p[4], p[11]);
          SWAP (p[5], p[10]);
          SWAP (p[6], p[9]);
          SWAP (p[7], p[8]);
          p += size;
        }
      break;

    case 8:
      while (nitems--)
        {
          SWAP (p[0], p[7]);
          SWAP (p[1], p[6]);
          SWAP (p[2], p[5]);
          SWAP (p[3], p[4]);
          p += size;
        }
      break;

    case 4:
      while (nitems--)
        {
          SWAP (p[0], p[3]);
          SWAP (p[1], p[2]);
          p += size;
        }
      break;

    case 2:
      while (nitems--)
        {
          SWAP (p[0], p[1]);
          p += size;
        }
      break;

    case 1:
      /* Nothing to do.  */
      break;

    default:
	_tt_abort("can't swap a %d-byte item for (un)dumping", size);
  }
}
#endif /* not WORDS_BIGENDIAN */


/* Here we write NITEMS items, each item being ITEM_SIZE bytes long.
   The pointer to the stuff to write is P, and we write to the file
   OUT_FILE.  */

static void
do_dump (char *p, int item_size, int nitems, rust_output_handle_t out_file)
{
#if !defined (WORDS_BIGENDIAN)
  swap_items (p, nitems, item_size);
#endif

  if (ttstub_output_write (out_file, p, item_size * nitems) != item_size * nitems)
      _tt_abort ("could not write %d %d-byte item(s) to %s",
		 nitems, item_size, name_of_file+1);

  /* Have to restore the old contents of memory, since some of it might
     get used again.  */
#if !defined (WORDS_BIGENDIAN)
  swap_items (p, nitems, item_size);
#endif
}


/* Here is the dual of the writing routine.  */

static void
do_undump (char *p, int item_size, int nitems, rust_input_handle_t in_file)
{
  if (ttstub_input_read (in_file, p, item_size * nitems) != item_size * nitems)
      _tt_abort("could not undump %d %d-byte item(s) from %s",
		nitems, item_size, name_of_file+1);

#if !defined (WORDS_BIGENDIAN)
  swap_items (p, nitems, item_size);
#endif
}


#define	dump_things(base, len) \
  do_dump ((char *) &(base), sizeof (base), (int) (len), fmt_out)
#define	undump_things(base, len) \
  do_undump ((char *) &(base), sizeof (base), (int) (len), fmt_in)

/* Like do_undump, but check each value against LOW and HIGH.  The
   slowdown isn't significant, and this improves the chances of
   detecting incompatible format files.  In fact, Knuth himself noted
   this problem with Web2c some years ago, so it seems worth fixing.  We
   can't make this a subroutine because then we lose the type of BASE.  */
#define undump_checked_things(low, high, base, len)			\
  do {                                                                  \
    unsigned i;                                                         \
    undump_things (base, len);                                           \
    for (i = 0; i < (len); i++) {                                       \
      if ((&(base))[i] < (low) || (&(base))[i] > (high)) {              \
        _tt_abort ("item %u (=%" PRIdPTR ") of .fmt array at %" PRIxPTR \
                " <%" PRIdPTR " or >%" PRIdPTR,                         \
                i, (uintptr_t) (&(base))[i], (uintptr_t) &(base),       \
                (uintptr_t) low, (uintptr_t) high);                     \
      }                                                                 \
    }									\
  } while (0)

/* Like undump_checked_things, but only check the upper value. We use
   this when the base type is unsigned, and thus all the values will be
   greater than zero by definition.  */
#define undump_upper_check_things(high, base, len)				\
  do {                                                                  \
    unsigned i;                                                         \
    undump_things (base, len);                                           \
    for (i = 0; i < (len); i++) {                                       \
      if ((&(base))[i] > (high)) {              			\
        _tt_abort ("Item %u (=%" PRIdPTR ") of .fmt array at %" PRIxPTR \
                " >%" PRIdPTR,                                          \
                i, (uintptr_t) (&(base))[i], (uintptr_t) &(base),       \
                (uintptr_t) high);                         		\
      }                                                                 \
    }									\
  } while (0)

/* Use the above for all the other dumping and undumping.  */
#define generic_dump(x) dump_things (x, 1)
#define generic_undump(x) undump_things (x, 1)

#define dump_wd   generic_dump
#define dump_hh   generic_dump
#define dump_qqqq generic_dump
#define undump_wd   generic_undump
#define undump_hh   generic_undump
#define	undump_qqqq generic_undump

/* `dump_int' is called with constant integers, so we put them into a
   variable first.  */
#define	dump_int(x)							\
  do									\
    {									\
      integer x_val = (x);						\
      generic_dump (x_val);						\
    }									\
  while (0)

#define	undump_int generic_undump


#define hash_offset 514
#define engine_name "xetex"
#define sup_main_memory 256000000L
#define sup_max_strings 2097151L
#define sup_font_mem_size 147483647L
#define sup_pool_size 40000000L
#define sup_string_vacancies (sup_pool_size - 23000)
#define sup_hash_extra sup_max_strings
#define inf_expand_depth 10
#define sup_expand_depth 10000000L


/*:134*//*135: */

static void
sort_avail(void)
{
    memory_word *mem = zmem;
    int32_t p, q, r;
    int32_t old_rover;

    p = get_node(1073741824L);
    p = mem[rover + 1].hh.v.RH;
    mem[rover + 1].hh.v.RH = 1073741823L;
    old_rover = rover;
    while (p != old_rover)      /*136: */
        if (p < rover) {
            q = p;
            p = mem[q + 1].hh.v.RH;
            mem[q + 1].hh.v.RH = rover;
            rover = q;
        } else {

            q = rover;
            while (mem[q + 1].hh.v.RH < p)
                q = mem[q + 1].hh.v.RH;
            r = mem[p + 1].hh.v.RH;
            mem[p + 1].hh.v.RH = mem[q + 1].hh.v.RH;
            mem[q + 1].hh.v.RH = p;
            p = r;
        }
    p = rover;
    while (mem[p + 1].hh.v.RH != 1073741823L) {

        mem[mem[p + 1].hh.v.RH + 1].hh.v.LH = p;
        p = mem[p + 1].hh.v.RH;
    }
    mem[p + 1].hh.v.RH = rover;
    mem[rover + 1].hh.v.LH = p;
}

/*:271*//*276: */

static void
primitive(str_number s, uint16_t c, int32_t o)
{
    memory_word *eqtb = zeqtb;
    pool_pointer k;
    integer j;
    small_number l;
    integer prim_val;

    if (s < 256) {
        cur_val = s + 1114113L;
        prim_val = s;
    } else {
        k = str_start[(s) - 65536L];
        l = str_start[(s + 1) - 65536L] - k;
        if (first + l > buf_size + 1)
            overflow(S(buffer_size), buf_size);

	for (j = 0; j <= l - 1; j++)
	    buffer[first + j] = str_pool[k + j];

        cur_val = id_lookup(first, l);
	str_ptr--;
	pool_ptr = str_start[(str_ptr) - 65536L];
        hash[cur_val].v.RH = s;
        prim_val = prim_lookup(s);
    }

    eqtb[cur_val].hh.u.B1 = 1 /*level_one */ ;
    eqtb[cur_val].hh.u.B0 = c;
    eqtb[cur_val].hh.v.RH = o;
    prim_eqtb[prim_val].hh.u.B1 = 1 /*level_one */ ;
    prim_eqtb[prim_val].hh.u.B0 = c;
    prim_eqtb[prim_val].hh.v.RH = o;
}

/*:925*//*977: */

trie_opcode znew_trie_op(small_number d, small_number n, trie_opcode v)
{
    register trie_opcode Result;
    new_trie_op_regmem integer h;
    trie_opcode u;
    integer l;
    h = abs(n + 313 * d + 361 * v + 1009 * cur_lang) % (trie_op_size - neg_trie_op_size) + neg_trie_op_size;
    while (true) {

        l = trie_op_hash[h];
        if (l == 0) {
            if (trie_op_ptr == trie_op_size)
                overflow(S(pattern_memory_ops), trie_op_size);
            u = trie_used[cur_lang];
            if (u == max_trie_op)
                overflow(S(pattern_memory_ops_per_langu/*age*/), max_trie_op - min_trie_op);
            trie_op_ptr++;
            u++;
            trie_used[cur_lang] = u;
            if (u > max_op_used)
                max_op_used = u;
            hyf_distance[trie_op_ptr] = d;
            hyf_num[trie_op_ptr] = n;
            hyf_next[trie_op_ptr] = v;
            trie_op_lang[trie_op_ptr] = cur_lang;
            trie_op_hash[h] = trie_op_ptr;
            trie_op_val[trie_op_ptr] = u;
            Result = u;
            return Result;
        }
        if ((hyf_distance[l] == d) && (hyf_num[l] == n) && (hyf_next[l] == v) && (trie_op_lang[l] == cur_lang)) {
            Result = trie_op_val[l];
            return Result;
        }
        if (h > -(integer) trie_op_size)
            h--;
        else
            h = trie_op_size;
    }
    return Result;
}

trie_pointer ztrie_node(trie_pointer p)
{
    register trie_pointer Result;
    trie_node_regmem trie_pointer h;
    trie_pointer q;
    h = abs(trie_c[p] + 1009 * trie_o[p] + 2718 * trie_l[p] + 3142 * trie_r[p]) % trie_size;
    while (true) {

        q = trie_hash[h];
        if (q == 0) {
            trie_hash[h] = p;
            Result = p;
            return Result;
        }
        if ((trie_c[q] == trie_c[p]) && (trie_o[q] == trie_o[p]) && (trie_l[q] == trie_l[p])
            && (trie_r[q] == trie_r[p])) {
            Result = q;
            return Result;
        }
        if (h > 0)
            h--;
        else
            h = trie_size;
    }
    return Result;
}

trie_pointer zcompress_trie(trie_pointer p)
{
    register trie_pointer Result;
    compress_trie_regmem if (p == 0)
        Result = 0;
    else {

        trie_l[p] = compress_trie(trie_l[p]);
        trie_r[p] = compress_trie(trie_r[p]);
        Result = trie_node(p);
    }
    return Result;
}

void zfirst_fit(trie_pointer p)
{
    first_fit_regmem trie_pointer h;
    trie_pointer z;
    trie_pointer q;
    UTF16_code c;
    trie_pointer l, r;
    integer /*too_big_char */ ll;
    c = trie_c[p];
    z = trie_min[c];
    while (true) {

        h = z - c;
        if (trie_max < h + max_hyph_char) {
            if (trie_size <= h + max_hyph_char)
                overflow(S(pattern_memory), trie_size);
            do {
                trie_max++;
                trie_taken[trie_max] = false;
                trie_trl[trie_max] = trie_max + 1;
                trie_tro[trie_max] = trie_max - 1;
            } while (!(trie_max == h + max_hyph_char));
        }
        if (trie_taken[h])
            goto lab45;
        q = trie_r[p];
        while (q > 0) {

            if (trie_trl[h + trie_c[q]] == 0)
                goto lab45;
            q = trie_r[q];
        }
        goto lab40;
 lab45:                        /*not_found */ z = trie_trl[z];
    }
 lab40:                        /*found *//*991: */ trie_taken[h] = true;
    trie_hash[p] = h;
    q = p;
    do {
        z = h + trie_c[q];
        l = trie_tro[z];
        r = trie_trl[z];
        trie_tro[r] = l;
        trie_trl[l] = r;
        trie_trl[z] = 0;
        if (l < max_hyph_char) {
            if (z < max_hyph_char)
                ll = z;
            else
                ll = max_hyph_char;
            do {
                trie_min[l] = r;
                l++;
            } while (!(l == ll));
        }
        q = trie_r[q];
    } while (!(q == 0 /*:991 */ ));
}

void ztrie_pack(trie_pointer p)
{
    trie_pack_regmem trie_pointer q;
    do {
        q = trie_l[p];
        if ((q > 0) && (trie_hash[q] == 0)) {
            first_fit(q);
            trie_pack(q);
        }
        p = trie_r[p];
    } while (!(p == 0));
}

void ztrie_fix(trie_pointer p)
{
    trie_fix_regmem trie_pointer q;
    UTF16_code c;
    trie_pointer z;
    z = trie_hash[p];
    do {
        q = trie_l[p];
        c = trie_c[p];
        trie_trl[z + c] = trie_hash[q];
        trie_trc[z + c] = c;
        trie_tro[z + c] = trie_o[p];
        if (q > 0)
            trie_fix(q);
        p = trie_r[p];
    } while (!(p == 0));
}

void new_patterns(void)
{
    new_patterns_regmem short /*hyphenatable_length_limit 1 */ k, l;
    boolean digit_sensed;
    trie_opcode v;
    trie_pointer p, q;
    boolean first_child;
    UTF16_code c;
    if (trie_not_ready) {
        if (eqtb[8938790L /*int_base 50 */ ].cint <= 0)
            cur_lang = 0;
        else if (eqtb[8938790L /*int_base 50 */ ].cint > 255 /*biggest_lang */ )
            cur_lang = 0;
        else
            cur_lang = eqtb[8938790L /*int_base 50 */ ].cint;
        scan_left_brace();
        k = 0;
        hyf[0] = 0;
        digit_sensed = false;
        while (true) {

            get_x_token();
            switch (cur_cmd) {
            case 11:
            case 12:
                if (digit_sensed || (cur_chr < 48 /*"0" */ ) || (cur_chr > 57 /*"9" */ )) {
                    if (cur_chr == 46 /*"." */ )
                        cur_chr = 0;
                    else {

                        cur_chr = eqtb[3368180L /*lc_code_base */  + cur_chr].hh.v.RH;
                        if (cur_chr == 0) {
                            {
                                if (interaction == 3 /*error_stop_mode */ ) ;
                                if (file_line_error_style_p)
                                    print_file_line();
                                else
                                    print_nl(S(__/*"! "*/));
                                print(S(Nonletter));
                            }
                            {
                                help_ptr = 1;
                                help_line[0] = S(_See_Appendix_H__);
                            }
                            error();
                        }
                    }
                    if (cur_chr > max_hyph_char)
                        max_hyph_char = cur_chr;
                    if (k < max_hyphenatable_length()) {
                        k++;
                        hc[k] = cur_chr;
                        hyf[k] = 0;
                        digit_sensed = false;
                    }
                } else if (k < max_hyphenatable_length()) {
                    hyf[k] = cur_chr - 48;
                    digit_sensed = true;
                }
                break;
            case 10:
            case 2:
                {
                    if (k > 0) {        /*998: */
                        if (hc[1] == 0)
                            hyf[0] = 0;
                        if (hc[k] == 0)
                            hyf[k] = 0;
                        l = k;
                        v = min_trie_op;
                        while (true) {

                            if (hyf[l] != 0)
                                v = new_trie_op(k - l, hyf[l], v);
                            if (l > 0)
                                l--;
                            else
                                goto lab31;
                        }
 lab31:                        /*done1 *//*:1000 */ ;
                        q = 0;
                        hc[0] = cur_lang;
                        while (l <= k) {

                            c = hc[l];
                            l++;
                            p = trie_l[q];
                            first_child = true;
                            while ((p > 0) && (c > trie_c[p])) {

                                q = p;
                                p = trie_r[q];
                                first_child = false;
                            }
                            if ((p == 0) || (c < trie_c[p])) {  /*999: */
                                if (trie_ptr == trie_size)
                                    overflow(S(pattern_memory), trie_size);
                                trie_ptr++;
                                trie_r[trie_ptr] = p;
                                p = trie_ptr;
                                trie_l[p] = 0;
                                if (first_child)
                                    trie_l[q] = p;
                                else
                                    trie_r[q] = p;
                                trie_c[p] = c;
                                trie_o[p] = min_trie_op;
                            }
                            q = p;
                        }
                        if (trie_o[q] != min_trie_op) {
                            {
                                if (interaction == 3 /*error_stop_mode */ ) ;
                                if (file_line_error_style_p)
                                    print_file_line();
                                else
                                    print_nl(S(__/*"! "*/));
                                print(S(Duplicate_pattern));
                            }
                            {
                                help_ptr = 1;
                                help_line[0] = S(_See_Appendix_H__);
                            }
                            error();
                        }
                        trie_o[q] = v;
                    }
                    if (cur_cmd == 2 /*right_brace */ )
                        goto lab30;
                    k = 0;
                    hyf[0] = 0;
                    digit_sensed = false;
                }
                break;
            default:
                {
                    {
                        if (interaction == 3 /*error_stop_mode */ ) ;
                        if (file_line_error_style_p)
                            print_file_line();
                        else
                            print_nl(S(__/*"! "*/));
                        print(S(Bad_));
                    }
                    print_esc(S(patterns));
                    {
                        help_ptr = 1;
                        help_line[0] = S(_See_Appendix_H__);
                    }
                    error();
                }
                break;
            }
        }
 lab30:                        /*done *//*:996 */ ;
        if (eqtb[8938806L /*int_base 66 */ ].cint > 0) {        /*1643: */
            c = cur_lang;
            first_child = false;
            p = 0;
            do {
                q = p;
                p = trie_r[q];
            } while (!((p == 0) || (c <= trie_c[p])));
            if ((p == 0) || (c < trie_c[p])) {  /*999: */
                if (trie_ptr == trie_size)
                    overflow(S(pattern_memory), trie_size);
                trie_ptr++;
                trie_r[trie_ptr] = p;
                p = trie_ptr;
                trie_l[p] = 0;
                if (first_child)
                    trie_l[q] = p;
                else
                    trie_r[q] = p;
                trie_c[p] = c;
                trie_o[p] = min_trie_op;
            }
            q = p;
            p = trie_l[q];
            first_child = true;
            {
                register integer for_end;
                c = 0;
                for_end = 255;
                if (c <= for_end)
                    do
                        if ((eqtb[3368180L /*lc_code_base */  + c].hh.v.RH > 0) || ((c == 255) && first_child)) {
                            if (p == 0) {       /*999: */
                                if (trie_ptr == trie_size)
                                    overflow(S(pattern_memory), trie_size);
                                trie_ptr++;
                                trie_r[trie_ptr] = p;
                                p = trie_ptr;
                                trie_l[p] = 0;
                                if (first_child)
                                    trie_l[q] = p;
                                else
                                    trie_r[q] = p;
                                trie_c[p] = c;
                                trie_o[p] = min_trie_op;
                            } else
                                trie_c[p] = c;
                            trie_o[p] = eqtb[3368180L /*lc_code_base */  + c].hh.v.RH;
                            q = p;
                            p = trie_r[q];
                            first_child = false;
                        }
                    while (c++ < for_end) ;
            }
            if (first_child)
                trie_l[q] = 0;
            else
                trie_r[q] = 0 /*:1644 */ ;
        }
    } else {

        {
            if (interaction == 3 /*error_stop_mode */ ) ;
            if (file_line_error_style_p)
                print_file_line();
            else
                print_nl(S(__/*"! "*/));
            print(S(Too_late_for_));
        }
        print_esc(S(patterns));
        {
            help_ptr = 1;
            help_line[0] = S(All_patterns_must_be_given_b/*efore typesetting begins.*/);
        }
        error();
        mem[mem_top - 12].hh.v.RH = scan_toks(false, false);
        flush_list(def_ref);
    }
}

void init_trie(void)
{
    init_trie_regmem trie_pointer p;
    integer j, k, t;
    trie_pointer r, s;
    max_hyph_char++;
    op_start[0] = -(integer) min_trie_op;
    {
        register integer for_end;
        j = 1;
        for_end = 255 /*biggest_lang */ ;
        if (j <= for_end)
            do
                op_start[j] = op_start[j - 1] + trie_used[j - 1];
            while (j++ < for_end);
    }
    {
        register integer for_end;
        j = 1;
        for_end = trie_op_ptr;
        if (j <= for_end)
            do
                trie_op_hash[j] = op_start[trie_op_lang[j]] + trie_op_val[j];
            while (j++ < for_end);
    }
    {
        register integer for_end;
        j = 1;
        for_end = trie_op_ptr;
        if (j <= for_end)
            do
                while (trie_op_hash[j] > j) {

                    k = trie_op_hash[j];
                    t = hyf_distance[k];
                    hyf_distance[k] = hyf_distance[j];
                    hyf_distance[j] = t;
                    t = hyf_num[k];
                    hyf_num[k] = hyf_num[j];
                    hyf_num[j] = t;
                    t = hyf_next[k];
                    hyf_next[k] = hyf_next[j];
                    hyf_next[j] = t;
                    trie_op_hash[j] = trie_op_hash[k];
                    trie_op_hash[k] = k;
                }
            while (j++ < for_end);
    }
    {
        register integer for_end;
        p = 0;
        for_end = trie_size;
        if (p <= for_end)
            do
                trie_hash[p] = 0;
            while (p++ < for_end);
    }
    trie_r[0] = compress_trie(trie_r[0]);
    trie_l[0] = compress_trie(trie_l[0]);
    {
        register integer for_end;
        p = 0;
        for_end = trie_ptr;
        if (p <= for_end)
            do
                trie_hash[p] = 0;
            while (p++ < for_end);
    }
    {
        register integer for_end;
        p = 0;
        for_end = 65535L /*biggest_char */ ;
        if (p <= for_end)
            do
                trie_min[p] = p + 1;
            while (p++ < for_end);
    }
    trie_trl[0] = 1;
    trie_max = 0 /*:987 */ ;
    if (trie_l[0] != 0) {
        first_fit(trie_l[0]);
        trie_pack(trie_l[0]);
    }
    if (trie_r[0] != 0) {       /*1645: */
        if (trie_l[0] == 0) {
            register integer for_end;
            p = 0;
            for_end = 255;
            if (p <= for_end)
                do
                    trie_min[p] = p + 2;
                while (p++ < for_end);
        }
        first_fit(trie_r[0]);
        trie_pack(trie_r[0]);
        hyph_start = trie_hash[trie_r[0]];
    }
    if (trie_max == 0) {
        {
            register integer for_end;
            r = 0;
            for_end = max_hyph_char;
            if (r <= for_end)
                do {
                    trie_trl[r] = 0;
                    trie_tro[r] = min_trie_op;
                    trie_trc[r] = 0;
                }
                while (r++ < for_end);
        }
        trie_max = max_hyph_char;
    } else {

        if (trie_r[0] > 0)
            trie_fix(trie_r[0]);
        if (trie_l[0] > 0)
            trie_fix(trie_l[0]);
        r = 0;
        do {
            s = trie_trl[r];
            {
                trie_trl[r] = 0;
                trie_tro[r] = min_trie_op;
                trie_trc[r] = 0;
            }
            r = s;
        } while (!(r > trie_max));
    }
    trie_trc[0] = 63 /*"?" */ ;
    trie_not_ready = false;
}

/*:1001*/

void new_hyph_exceptions(void)
{
    new_hyph_exceptions_regmem short /*hyphenatable_length_limit 1 */ n;
    short /*hyphenatable_length_limit 1 */ j;
    hyph_pointer h;
    str_number k;
    int32_t p;
    int32_t q;
    str_number s;
    pool_pointer u, v;
    scan_left_brace();
    if (eqtb[8938790L /*int_base 50 */ ].cint <= 0)
        cur_lang = 0;
    else if (eqtb[8938790L /*int_base 50 */ ].cint > 255 /*biggest_lang */ )
        cur_lang = 0;
    else
        cur_lang = eqtb[8938790L /*int_base 50 */ ].cint;

    if (trie_not_ready) {
        hyph_index = 0;
        goto lab46;
    }

    if (trie_trc[hyph_start + cur_lang] != cur_lang)
        hyph_index = 0;
    else
        hyph_index = trie_trl[hyph_start + cur_lang];
 lab46:                        /*not_found1 *//*970: */ n = 0;
    p = -268435455L;
    while (true) {

        get_x_token();
 lab21:                        /*reswitch */ switch (cur_cmd) {
        case 11:
        case 12:
        case 68:
            if (cur_chr == 45 /*"-" */ ) {      /*973: */
                if (n < max_hyphenatable_length()) {
                    q = get_avail();
                    mem[q].hh.v.RH = p;
                    mem[q].hh.v.LH = n;
                    p = q;
                }
            } else {

                if ((hyph_index == 0) || ((cur_chr) > 255))
                    hc[0] = eqtb[3368180L /*lc_code_base */  + cur_chr].hh.v.RH;
                else if (trie_trc[hyph_index + cur_chr] != cur_chr)
                    hc[0] = 0;
                else
                    hc[0] = trie_tro[hyph_index + cur_chr];
                if (hc[0] == 0) {
                    {
                        if (interaction == 3 /*error_stop_mode */ ) ;
                        if (file_line_error_style_p)
                            print_file_line();
                        else
                            print_nl(S(__/*"! "*/));
                        print(S(Not_a_letter));
                    }
                    {
                        help_ptr = 2;
                        help_line[1] = S(Letters_in__hyphenation_word/*s must have \lccode>0.*/);
                        help_line[0] = S(Proceed__I_ll_ignore_the_cha/*racter I just read.*/);
                    }
                    error();
                } else if (n < max_hyphenatable_length()) {
                    n++;
                    if (hc[0] < 65536L)
                        hc[n] = hc[0];
                    else {

                        hc[n] = (hc[0] - 65536L) / 1024 + 55296L;
                        n++;
                        hc[n] = hc[0] % 1024 + 56320L;
                    }
                }
            }
            break;
        case 16:
            {
                scan_char_num();
                cur_chr = cur_val;
                cur_cmd = 68 /*char_given */ ;
                goto lab21;
            }
            break;
        case 10:
        case 2:
            {
                if (n > 1) {    /*974: */
                    n++;
                    hc[n] = cur_lang;
                    {
                        if (pool_ptr + n > pool_size)
                            overflow(S(pool_size), pool_size - init_pool_ptr);
                    }
                    h = 0;
                    {
                        register integer for_end;
                        j = 1;
                        for_end = n;
                        if (j <= for_end)
                            do {
                                h = (h + h + hc[j]) % 607 /*hyph_prime */ ;
                                {
                                    str_pool[pool_ptr] = hc[j];
                                    pool_ptr++;
                                }
                            }
                            while (j++ < for_end);
                    }
                    s = make_string();
                    if (hyph_next <= 607 /*hyph_prime */ )
                        while ((hyph_next > 0) && (hyph_word[hyph_next - 1] > 0))
                            hyph_next--;
                    if ((hyph_count == hyph_size) || (hyph_next == 0))
                        overflow(S(exception_dictionary), hyph_size);
                    hyph_count++;
                    while (hyph_word[h] != 0) {

                        k = hyph_word[h];
                        if (length(k) != length(s))
                            goto lab45;
                        u = str_start[(k) - 65536L];
                        v = str_start[(s) - 65536L];
                        do {
                            if (str_pool[u] != str_pool[v])
                                goto lab45;
                            u++;
                            v++;
                        } while (!(u == str_start[(k + 1) - 65536L]));
                        {
                            str_ptr--;
                            pool_ptr = str_start[(str_ptr) - 65536L];
                        }
                        s = hyph_word[h];
                        hyph_count--;
                        goto lab40;
 lab45:                        /*not_found *//*:976 */ ;
                        if (hyph_link[h] == 0) {
                            hyph_link[h] = hyph_next;
                            if (hyph_next >= hyph_size)
                                hyph_next = 607 /*hyph_prime */ ;
                            if (hyph_next > 607 /*hyph_prime */ )
                                hyph_next++;
                        }
                        h = hyph_link[h] - 1;
                    }
 lab40:                        /*found */ hyph_word[h] = s;
                    hyph_list[h] = /*:975 */ p;
                }
                if (cur_cmd == 2 /*right_brace */ )
                    return;
                n = 0;
                p = -268435455L;
            }
            break;
        default:
            {
                {
                    if (interaction == 3 /*error_stop_mode */ ) ;
                    if (file_line_error_style_p)
                        print_file_line();
                    else
                        print_nl(S(__/*"! "*/));
                    print(S(Improper_));
                }
                print_esc(S(hyphenation));
                print(S(_will_be_flushed));
                {
                    help_ptr = 2;
                    help_line[1] = S(Hyphenation_exceptions_must_/*contain only letters*/);
                    help_line[0] = S(and_hyphens__But_continue__I/*'ll forgive and forget.*/);
                }
                error();
            }
            break;
        }
    }
}

void prefixed_command(void)
{
    prefixed_command_regmem small_number a;
    internal_font_number f;
    int32_t j;
    font_index k;
    int32_t p, q;
    integer n;
    boolean e;
    a = 0;
    while (cur_cmd == 95 /*prefix */ ) {

        if (!odd(a / cur_chr))
            a = a + cur_chr;
        do {
            get_x_token();
        } while (!((cur_cmd != 10 /*spacer */ ) && (cur_cmd != 0 /*relax */ ) /*:422 */ ));
        if (cur_cmd <= 71 /*max_non_prefixed_command */ ) {     /*1247: */
            {
                if (interaction == 3 /*error_stop_mode */ ) ;
                if (file_line_error_style_p)
                    print_file_line();
                else
                    print_nl(S(__/*"! "*/));
                print(S(You_can_t_use_a_prefix_with_/*`*/));
            }
            print_cmd_chr(cur_cmd, cur_chr);
            print_char(39 /*"'" */ );
            {
                help_ptr = 1;
                help_line[0] = S(I_ll_pretend_you_didn_t_say_/*\long or \outer or \global.*/);
            }
            if ((eTeX_mode == 1))
                help_line[0] = S(I_ll_pretend_you_didn_t_say__Z1/*"I'll pretend you didn't say \long or \outer or \global or \protected."*/);
            back_error();
            return;
        }
        if (eqtb[8938776L /*int_base 36 */ ].cint > 2) {

            if ((eTeX_mode == 1))
                show_cur_cmd_chr();
        }
    }
    if (a >= 8) {
        j = 29360129L /*protected_token */ ;
        a = a - 8;
    } else
        j = 0;
    if ((cur_cmd != 99 /*def */ ) && ((a % 4 != 0) || (j != 0))) {
        {
            if (interaction == 3 /*error_stop_mode */ ) ;
            if (file_line_error_style_p)
                print_file_line();
            else
                print_nl(S(__/*"! "*/));
            print(S(You_can_t_use__));
        }
        print_esc(S(long));
        print(S(__or__/*"' or `"*/));
        print_esc(S(outer));
        {
            help_ptr = 1;
            help_line[0] = S(I_ll_pretend_you_didn_t_say__Z2/*"I'll pretend you didn't say \long or \outer here."*/);
        }
        if ((eTeX_mode == 1)) {
            help_line[0] = S(I_ll_pretend_you_didn_t_say__Z3/*"I'll pretend you didn't say \long or \outer or \protected here."*/);
            print(S(__or__/*"' or `"*/));
            print_esc(S(protected));
        }
        print(S(__with__));
        print_cmd_chr(cur_cmd, cur_chr);
        print_char(39 /*"'" */ );
        error();
    }
    if (eqtb[8938783L /*int_base 43 */ ].cint != 0) {

        if (eqtb[8938783L /*int_base 43 */ ].cint < 0) {
            if ((a >= 4))
                a = a - 4;
        } else {

            if (!(a >= 4))
                a = a + 4;
        }
    }
    switch (cur_cmd) {          /*1252: */
    case 89:
        if ((a >= 4))
            geq_define(2253299L /*cur_font_loc */ , 122 /*data */ , cur_chr);
        else
            eq_define(2253299L /*cur_font_loc */ , 122 /*data */ , cur_chr);
        break;
    case 99:
        {
            if (odd(cur_chr) && !(a >= 4) && (eqtb[8938783L /*int_base 43 */ ].cint >= 0))
                a = a + 4;
            e = (cur_chr >= 2);
            get_r_token();
            p = cur_cs;
            q = scan_toks(true, e);
            if (j != 0) {
                q = get_avail();
                mem[q].hh.v.LH = j;
                mem[q].hh.v.RH = mem[def_ref].hh.v.RH;
                mem[def_ref].hh.v.RH = q;
            }
            if ((a >= 4))
                geq_define(p, 113 /*call */  + (a % 4), def_ref);
            else
                eq_define(p, 113 /*call */  + (a % 4), def_ref);
        }
        break;
    case 96:
        {
            n = cur_chr;
            get_r_token();
            p = cur_cs;
            if (n == 0 /*normal */ ) {
                do {
                    get_token();
                } while (!(cur_cmd != 10 /*spacer */ ));
                if (cur_tok == 25165885L /*other_token 61 */ ) {
                    get_token();
                    if (cur_cmd == 10 /*spacer */ )
                        get_token();
                }
            } else {

                get_token();
                q = cur_tok;
                get_token();
                back_input();
                cur_tok = q;
                back_input();
            }
            if (cur_cmd >= 113 /*call */ )
                mem[cur_chr].hh.v.LH++;
            else if ((cur_cmd == 91 /*register */ ) || (cur_cmd == 72 /*toks_register */ )) {

                if ((cur_chr < mem_bot) || (cur_chr > mem_bot + 19))
                    mem[cur_chr + 1].hh.v.LH++;
            }
            if ((a >= 4))
                geq_define(p, cur_cmd, cur_chr);
            else
                eq_define(p, cur_cmd, cur_chr);
        }
        break;
    case 97:
        if (cur_chr == 7 /*char_sub_def_code */ ) {
            scan_char_num();
            p = 7824628L /*char_sub_code_base */  + cur_val;
            scan_optional_equals();
            scan_char_num();
            n = cur_val;
            scan_char_num();
            if ((eqtb[8938797L /*int_base 57 */ ].cint > 0)) {
                begin_diagnostic();
                print_nl(S(New_character_substitution__/**/));
                print(p - 7824628L);
                print(S(____Z6/*" = "*/));
                print(n);
                print_char(32 /*" " */ );
                print(cur_val);
                end_diagnostic(false);
            }
            n = n * 256 + cur_val;
            if ((a >= 4))
                geq_define(p, 122 /*data */ , n);
            else
                eq_define(p, 122 /*data */ , n);
            if ((p - 7824628L) < eqtb[8938795L /*int_base 55 */ ].cint) {

                if ((a >= 4))
                    geq_word_define(8938795L /*int_base 55 */ , p - 7824628L);
                else
                    eq_word_define(8938795L /*int_base 55 */ , p - 7824628L);
            }
            if ((p - 7824628L) > eqtb[8938796L /*int_base 56 */ ].cint) {

                if ((a >= 4))
                    geq_word_define(8938796L /*int_base 56 */ , p - 7824628L);
                else
                    eq_word_define(8938796L /*int_base 56 */ , p - 7824628L);
            }
        } else {

            n = cur_chr;
            get_r_token();
            p = cur_cs;
            if ((a >= 4))
                geq_define(p, 0 /*relax */ , 1114112L /*too_big_usv */ );
            else
                eq_define(p, 0 /*relax */ , 1114112L /*too_big_usv */ );
            scan_optional_equals();
            switch (n) {
            case 0:
                {
                    scan_usv_num();
                    if ((a >= 4))
                        geq_define(p, 68 /*char_given */ , cur_val);
                    else
                        eq_define(p, 68 /*char_given */ , cur_val);
                }
                break;
            case 1:
                {
                    scan_fifteen_bit_int();
                    if ((a >= 4))
                        geq_define(p, 69 /*math_given */ , cur_val);
                    else
                        eq_define(p, 69 /*math_given */ , cur_val);
                }
                break;
            case 8:
                {
                    scan_xetex_math_char_int();
                    if ((a >= 4))
                        geq_define(p, 70 /*XeTeX_math_given */ , cur_val);
                    else
                        eq_define(p, 70 /*XeTeX_math_given */ , cur_val);
                }
                break;
            case 9:
                {
                    scan_math_class_int();
                    n = set_class(cur_val);
                    scan_math_fam_int();
                    n = n + set_family(cur_val);
                    scan_usv_num();
                    n = n + cur_val;
                    if ((a >= 4))
                        geq_define(p, 70 /*XeTeX_math_given */ , n);
                    else
                        eq_define(p, 70 /*XeTeX_math_given */ , n);
                }
                break;
            default:
                {
                    scan_register_num();
                    if (cur_val > 255) {
                        j = n - 2;
                        if (j > 3 /*mu_val */ )
                            j = 5 /*tok_val */ ;
                        find_sa_element(j, cur_val, true);
                        mem[cur_ptr + 1].hh.v.LH++;
                        if (j == 5 /*tok_val */ )
                            j = 72 /*toks_register */ ;
                        else
                            j = 91 /*register */ ;
                        if ((a >= 4))
                            geq_define(p, j, cur_ptr);
                        else
                            eq_define(p, j, cur_ptr);
                    } else
                        switch (n) {
                        case 2:
                            if ((a >= 4))
                                geq_define(p, 74 /*assign_int */ , 8938824L /*count_base */  + cur_val);
                            else
                                eq_define(p, 74 /*assign_int */ , 8938824L /*count_base */  + cur_val);
                            break;
                        case 3:
                            if ((a >= 4))
                                geq_define(p, 75 /*assign_dimen */ , 10053215L /*scaled_base */  + cur_val);
                            else
                                eq_define(p, 75 /*assign_dimen */ , 10053215L /*scaled_base */  + cur_val);
                            break;
                        case 4:
                            if ((a >= 4))
                                geq_define(p, 76 /*assign_glue */ , 2252259L /*skip_base */  + cur_val);
                            else
                                eq_define(p, 76 /*assign_glue */ , 2252259L /*skip_base */  + cur_val);
                            break;
                        case 5:
                            if ((a >= 4))
                                geq_define(p, 77 /*assign_mu_glue */ , 2252515L /*mu_skip_base */  + cur_val);
                            else
                                eq_define(p, 77 /*assign_mu_glue */ , 2252515L /*mu_skip_base */  + cur_val);
                            break;
                        case 6:
                            if ((a >= 4))
                                geq_define(p, 73 /*assign_toks */ , 2252783L /*toks_base */  + cur_val);
                            else
                                eq_define(p, 73 /*assign_toks */ , 2252783L /*toks_base */  + cur_val);
                            break;
                        }
                }
                break;
            }
        }
        break;
    case 98:
        {
            j = cur_chr;
            scan_int();
            n = cur_val;
            if (!scan_keyword(S(to))) {
                {
                    if (interaction == 3 /*error_stop_mode */ ) ;
                    if (file_line_error_style_p)
                        print_file_line();
                    else
                        print_nl(S(__/*"! "*/));
                    print(S(Missing__to__inserted));
                }
                {
                    help_ptr = 2;
                    help_line[1] = S(You_should_have_said___read_/*number> to \cs'.*/);
                    help_line[0] = S(I_m_going_to_look_for_the__c/*s now.*/);
                }
                error();
            }
            get_r_token();
            p = cur_cs;
            read_toks(n, p, j);
            if ((a >= 4))
                geq_define(p, 113 /*call */ , cur_val);
            else
                eq_define(p, 113 /*call */ , cur_val);
        }
        break;
    case 72:
    case 73:
        {
            q = cur_cs;
            e = false;
            if (cur_cmd == 72 /*toks_register */ ) {

                if (cur_chr == mem_bot) {
                    scan_register_num();
                    if (cur_val > 255) {
                        find_sa_element(5 /*tok_val */ , cur_val, true);
                        cur_chr = cur_ptr;
                        e = true;
                    } else
                        cur_chr = 2252783L /*toks_base */  + cur_val;
                } else
                    e = true;
            } else if (cur_chr == 2252782L /*XeTeX_inter_char_loc */ ) {
                scan_char_class_not_ignored();
                cur_ptr = cur_val;
                scan_char_class_not_ignored();
                find_sa_element(6 /*inter_char_val */ , cur_ptr * 4096 /*char_class_limit */  + cur_val, true);
                cur_chr = cur_ptr;
                e = true;
            }
            p = cur_chr;
            scan_optional_equals();
            do {
                get_x_token();
            } while (!((cur_cmd != 10 /*spacer */ ) && (cur_cmd != 0 /*relax */ ) /*:422 */ ));
            if (cur_cmd != 1 /*left_brace */ ) {        /*1262: */

                if ((cur_cmd == 72 /*toks_register */ ) || (cur_cmd == 73 /*assign_toks */ )) {
                    if (cur_cmd == 72 /*toks_register */ ) {

                        if (cur_chr == mem_bot) {
                            scan_register_num();
                            if (cur_val < 256)
                                q = eqtb[2252783L /*toks_base */  + cur_val].hh.v.RH;
                            else {

                                find_sa_element(5 /*tok_val */ , cur_val, false);
                                if (cur_ptr == -268435455L)
                                    q = -268435455L;
                                else
                                    q = mem[cur_ptr + 1].hh.v.RH;
                            }
                        } else
                            q = mem[cur_chr + 1].hh.v.RH;
                    } else if (cur_chr == 2252782L /*XeTeX_inter_char_loc */ ) {
                        scan_char_class_not_ignored();
                        cur_ptr = cur_val;
                        scan_char_class_not_ignored();
                        find_sa_element(6 /*inter_char_val */ , cur_ptr * 4096 /*char_class_limit */  + cur_val, false);
                        if (cur_ptr == -268435455L)
                            q = -268435455L;
                        else
                            q = mem[cur_ptr + 1].hh.v.RH;
                    } else
                        q = eqtb[cur_chr].hh.v.RH;
                    if (q == -268435455L) {

                        if (e) {

                            if ((a >= 4))
                                gsa_def(p, -268435455L);
                            else
                                sa_def(p, -268435455L);
                        } else if ((a >= 4))
                            geq_define(p, 103 /*undefined_cs */ , -268435455L);
                        else
                            eq_define(p, 103 /*undefined_cs */ , -268435455L);
                    } else {

                        mem[q].hh.v.LH++;
                        if (e) {

                            if ((a >= 4))
                                gsa_def(p, q);
                            else
                                sa_def(p, q);
                        } else if ((a >= 4))
                            geq_define(p, 113 /*call */ , q);
                        else
                            eq_define(p, 113 /*call */ , q);
                    }
                    goto lab30;
                }
            }
            back_input();
            cur_cs = q;
            q = scan_toks(false, false);
            if (mem[def_ref].hh.v.RH == -268435455L) {
                if (e) {

                    if ((a >= 4))
                        gsa_def(p, -268435455L);
                    else
                        sa_def(p, -268435455L);
                } else if ((a >= 4))
                    geq_define(p, 103 /*undefined_cs */ , -268435455L);
                else
                    eq_define(p, 103 /*undefined_cs */ , -268435455L);
                {
                    mem[def_ref].hh.v.RH = avail;
                    avail = def_ref;
                }
            } else {

                if ((p == 2252772L /*output_routine_loc */ ) && !e) {
                    mem[q].hh.v.RH = get_avail();
                    q = mem[q].hh.v.RH;
                    mem[q].hh.v.LH = 4194429L /*right_brace_token 125 */ ;
                    q = get_avail();
                    mem[q].hh.v.LH = 2097275L /*left_brace_token 123 */ ;
                    mem[q].hh.v.RH = mem[def_ref].hh.v.RH;
                    mem[def_ref].hh.v.RH = q;
                }
                if (e) {

                    if ((a >= 4))
                        gsa_def(p, def_ref);
                    else
                        sa_def(p, def_ref);
                } else if ((a >= 4))
                    geq_define(p, 113 /*call */ , def_ref);
                else
                    eq_define(p, 113 /*call */ , def_ref);
            }
        }
        break;
    case 74:
        {
            p = cur_chr;
            scan_optional_equals();
            scan_int();
            if ((a >= 4))
                geq_word_define(p, cur_val);
            else
                eq_word_define(p, cur_val);
        }
        break;
    case 75:
        {
            p = cur_chr;
            scan_optional_equals();
            scan_dimen(false, false, false);
            if ((a >= 4))
                geq_word_define(p, cur_val);
            else
                eq_word_define(p, cur_val);
        }
        break;
    case 76:
    case 77:
        {
            p = cur_chr;
            n = cur_cmd;
            scan_optional_equals();
            if (n == 77 /*assign_mu_glue */ )
                scan_glue(3 /*mu_val */ );
            else
                scan_glue(2 /*glue_val */ );
            trap_zero_glue();
            if ((a >= 4))
                geq_define(p, 119 /*glue_ref */ , cur_val);
            else
                eq_define(p, 119 /*glue_ref */ , cur_val);
        }
        break;
    case 87:
        {
            if (cur_chr == 5596404L /*sf_code_base */ ) {
                p = cur_chr;
                scan_usv_num();
                p = p + cur_val;
                n = eqtb[5596404L /*sf_code_base */  + cur_val].hh.v.RH % 65536L;
                scan_optional_equals();
                scan_char_class();
                if ((a >= 4))
                    geq_define(p, 122 /*data */ , cur_val * 65536L + n);
                else
                    eq_define(p, 122 /*data */ , cur_val * 65536L + n);
            } else if (cur_chr == 6710516L /*math_code_base */ ) {
                p = cur_chr;
                scan_usv_num();
                p = p + cur_val;
                scan_optional_equals();
                scan_xetex_math_char_int();
                if ((a >= 4))
                    geq_define(p, 122 /*data */ , cur_val);
                else
                    eq_define(p, 122 /*data */ , cur_val);
            } else if (cur_chr == 6710517L /*math_code_base 1 */ ) {
                p = cur_chr - 1;
                scan_usv_num();
                p = p + cur_val;
                scan_optional_equals();
                scan_math_class_int();
                n = set_class(cur_val);
                scan_math_fam_int();
                n = n + set_family(cur_val);
                scan_usv_num();
                n = n + cur_val;
                if ((a >= 4))
                    geq_define(p, 122 /*data */ , n);
                else
                    eq_define(p, 122 /*data */ , n);
            } else if (cur_chr == 8939080L /*del_code_base */ ) {
                p = cur_chr;
                scan_usv_num();
                p = p + cur_val;
                scan_optional_equals();
                scan_int();
                if ((a >= 4))
                    geq_word_define(p, cur_val);
                else
                    eq_word_define(p, cur_val);
            } else {

                p = cur_chr - 1;
                scan_usv_num();
                p = p + cur_val;
                scan_optional_equals();
                n = 1073741824L;
                scan_math_fam_int();
                n = n + cur_val * 2097152L;
                scan_usv_num();
                n = n + cur_val;
                if ((a >= 4))
                    geq_word_define(p, n);
                else
                    eq_word_define(p, n);
            }
        }
        break;
    case 86:
        {
            if (cur_chr == 2254068L /*cat_code_base */ )
                n = 15 /*max_char_code */ ;
            else if (cur_chr == 6710516L /*math_code_base */ )
                n = 32768L;
            else if (cur_chr == 5596404L /*sf_code_base */ )
                n = 32767;
            else if (cur_chr == 8939080L /*del_code_base */ )
                n = 16777215L;
            else
                n = 1114111L /*biggest_usv *//*:1268 */ ;
            p = cur_chr;
            scan_usv_num();
            p = p + cur_val;
            scan_optional_equals();
            scan_int();
            if (((cur_val < 0) && (p < 8939080L /*del_code_base */ )) || (cur_val > n)) {
                {
                    if (interaction == 3 /*error_stop_mode */ ) ;
                    if (file_line_error_style_p)
                        print_file_line();
                    else
                        print_nl(S(__/*"! "*/));
                    print(S(Invalid_code__));
                }
                print_int(cur_val);
                if (p < 8939080L /*del_code_base */ )
                    print(S(___should_be_in_the_range_0_/*.*/));
                else
                    print(S(___should_be_at_most_));
                print_int(n);
                {
                    help_ptr = 1;
                    help_line[0] = S(I_m_going_to_use_0_instead_o/*f that illegal code value.*/);
                }
                error();
                cur_val = 0;
            }
            if (p < 6710516L /*math_code_base */ ) {
                if (p >= 5596404L /*sf_code_base */ ) {
                    n = eqtb[p].hh.v.RH / 65536L;
                    if ((a >= 4))
                        geq_define(p, 122 /*data */ , n * 65536L + cur_val);
                    else
                        eq_define(p, 122 /*data */ , n * 65536L + cur_val);
                } else if ((a >= 4))
                    geq_define(p, 122 /*data */ , cur_val);
                else
                    eq_define(p, 122 /*data */ , cur_val);
            } else if (p < 8939080L /*del_code_base */ ) {
                if (cur_val == 32768L)
                    cur_val = 2097151L /*active_math_char */ ;
                else
                    cur_val =
                        set_class(cur_val / 4096) + set_family((cur_val % 4096) / 256) + (cur_val % 256);
                if ((a >= 4))
                    geq_define(p, 122 /*data */ , cur_val);
                else
                    eq_define(p, 122 /*data */ , cur_val);
            } else if ((a >= 4))
                geq_word_define(p, cur_val);
            else
                eq_word_define(p, cur_val);
        }
        break;
    case 88:
        {
            p = cur_chr;
            scan_math_fam_int();
            p = p + cur_val;
            scan_optional_equals();
            scan_font_ident();
            if ((a >= 4))
                geq_define(p, 122 /*data */ , cur_val);
            else
                eq_define(p, 122 /*data */ , cur_val);
        }
        break;
    case 91:
    case 92:
    case 93:
    case 94:
        do_register_command(a);
        break;
    case 100:
        {
            scan_register_num();
            if ((a >= 4))
                n = 1073774592L + cur_val;
            else
                n = 1073741824L + cur_val;
            scan_optional_equals();
            if (set_box_allowed)
                scan_box(n);
            else {

                {
                    if (interaction == 3 /*error_stop_mode */ ) ;
                    if (file_line_error_style_p)
                        print_file_line();
                    else
                        print_nl(S(__/*"! "*/));
                    print(S(Improper_));
                }
                print_esc(S(setbox));
                {
                    help_ptr = 2;
                    help_line[1] = S(Sorry___setbox_is_not_allowe/*d after \halign in a display,*/);
                    help_line[0] = S(or_between__accent_and_an_ac/*cented character.*/);
                }
                error();
            }
        }
        break;
    case 80:
        alter_aux();
        break;
    case 81:
        alter_prev_graf();
        break;
    case 82:
        alter_page_so_far();
        break;
    case 83:
        alter_integer();
        break;
    case 84:
        alter_box_dimen();
        break;
    case 85:
        {
            q = cur_chr;
            scan_optional_equals();
            scan_int();
            n = cur_val;
            if (n <= 0)
                p = -268435455L;
            else if (q > 2252771L /*par_shape_loc */ ) {
                n = (cur_val / 2) + 1;
                p = get_node(2 * n + 1);
                mem[p].hh.v.LH = n;
                n = cur_val;
                mem[p + 1].cint = n;
                {
                    register integer for_end;
                    j = p + 2;
                    for_end = p + n + 1;
                    if (j <= for_end)
                        do {
                            scan_int();
                            mem[j].cint = cur_val;
                        }
                        while (j++ < for_end);
                }
                if (!odd(n))
                    mem[p + n + 2].cint = 0;
            } else {

                p = get_node(2 * n + 1);
                mem[p].hh.v.LH = n;
                {
                    register integer for_end;
                    j = 1;
                    for_end = n;
                    if (j <= for_end)
                        do {
                            scan_dimen(false, false, false);
                            mem[p + 2 * j - 1].cint = cur_val;
                            scan_dimen(false, false, false);
                            mem[p + 2 * j].cint = cur_val;
                        }
                        while (j++ < for_end);
                }
            }
            if ((a >= 4))
                geq_define(q, 120 /*shape_ref */ , p);
            else
                eq_define(q, 120 /*shape_ref */ , p);
        }
        break;
    case 101:
        if (cur_chr == 1) {
            if (in_initex_mode) {
                new_patterns();
                goto lab30;
            }
            {
                if (interaction == 3 /*error_stop_mode */ ) ;
                if (file_line_error_style_p)
                    print_file_line();
                else
                    print_nl(S(__/*"! "*/));
                print(S(Patterns_can_be_loaded_only_/*by INITEX*/));
            }
            help_ptr = 0;
            error();
            do {
                get_token();
            } while (!(cur_cmd == 2 /*right_brace */ ));
            return;
        } else {

            new_hyph_exceptions();
            goto lab30;
        }
        break;
    case 78:
        {
            find_font_dimen(true);
            k = cur_val;
            scan_optional_equals();
            scan_dimen(false, false, false);
            font_info[k].cint = cur_val;
        }
        break;
    case 79:
        {
            n = cur_chr;
            scan_font_ident();
            f = cur_val;
            if (n < 2) {
                scan_optional_equals();
                scan_int();
                if (n == 0)
                    hyphen_char[f] = cur_val;
                else
                    skew_char[f] = cur_val;
            } else {

                if (((font_area[f] == 65535L /*aat_font_flag */ ) || (font_area[f] == 65534L /*otgr_font_flag */ )))
                    scan_glyph_number(f);
                else
                    scan_char_num();
                p = cur_val;
                scan_optional_equals();
                scan_int();
                switch (n) {
                case 2:
                    set_cp_code(f, p, 0, cur_val);
                    break;
                case 3:
                    set_cp_code(f, p, 1, cur_val);
                    break;
                }
            }
        }
        break;
    case 90:
        new_font(a);
        break;
    case 102:
        new_interaction();
        break;
    default:
        confusion(S(prefix));
        break;
    }
 lab30:                        /*done *//*1304: */ if (after_token != 0) {
        cur_tok = after_token;
        back_input();
        after_token = 0;
    }
}

/*:1328*//*1337: */

static void
store_fmt_file(void)
{
    memory_word *mem = zmem, *eqtb = zeqtb;
    integer j, k, l;
    int32_t p, q;
    integer x;
    char *format_engine;
    rust_output_handle_t fmt_out;

    if (save_ptr != 0) {
	if (file_line_error_style_p)
	    print_file_line();
	else
	    print_nl(S(__/*"! "*/));
	print(S(You_can_t_dump_inside_a_grou/*p*/));
	help_ptr = 1;
	help_line[0] = 66678L /*"`_...\dump_' is a no-no." */ ;

	if (interaction == 3 /*error_stop_mode */ )
	    interaction = 2 /*scroll_mode */ ;
	if (log_opened)
	    error();
	history = HISTORY_FATAL_ERROR;
	close_files_and_terminate();
	ttstub_output_flush (rust_stdout);
	_tt_abort("\\dump inside a group");
    }

    selector = SELECTOR_NEW_STRING;
    print(S(__preloaded_format_));
    print(job_name);
    print_char(32 /*" " */ );
    print_int(eqtb[8938763L /*int_base 23 */ ].cint);
    print_char(46 /*"." */ );
    print_int(eqtb[8938762L /*int_base 22 */ ].cint);
    print_char(46 /*"." */ );
    print_int(eqtb[8938761L /*int_base 21 */ ].cint);
    print_char(41 /*")" */ );
    if (interaction == 0 /*batch_mode */ )
        selector = SELECTOR_LOG_ONLY;
    else
        selector = SELECTOR_TERM_AND_LOG;
    {
        if (pool_ptr + 1 > pool_size)
            overflow(S(pool_size), pool_size - init_pool_ptr);
    }
    format_ident = make_string();
    pack_job_name(66141L /*format_extension */ );

    fmt_out = ttstub_output_open (name_of_file + 1, 1);
    if (fmt_out == NULL)
	_tt_abort ("cannot open format output file \"%s\"", name_of_file + 1);

    print_nl(S(Beginning_to_dump_on_file_));
    print(make_name_string());
    {
        str_ptr--;
        pool_ptr = str_start[(str_ptr) - 65536L];
    }
    print_nl(S());
    print(format_ident);
    dump_int(1462916184L);
    x = strlen(engine_name);
    format_engine = xmalloc_array(char, x + 4);
    strcpy((string) (format_engine), engine_name);
    {
        register integer for_end;
        k = x;
        for_end = x + 3;
        if (k <= for_end)
            do
                format_engine[k] = 0;
            while (k++ < for_end);
    }
    x = x + 4 - (x % 4);
    dump_int(x);
    dump_things(format_engine[0], x);
    free(format_engine);
    dump_int(457477274L);
    dump_int(1073741823L);
    dump_int(hash_high);
    dump_int(eTeX_mode);
    while (pseudo_files != -268435455L)
        pseudo_close();
    dump_int(mem_bot);
    dump_int(mem_top);
    dump_int(10053470L /*eqtb_size */ );
    dump_int(8501 /*hash_prime */ );
    dump_int(607 /*hyph_prime */ );
    dump_int(1296847960L);
    if (mltex_p)
        dump_int(1);
    else
        dump_int(0);
    dump_int(pool_ptr);
    dump_int(str_ptr);
    dump_things(str_start[(65536L /*too_big_char */ ) - 65536L], str_ptr - 65535L);
    dump_things(str_pool[0], pool_ptr);
    print_ln();
    print_int(str_ptr);
    print(S(_strings_of_total_length_));
    print_int(pool_ptr);
    sort_avail();
    var_used = 0;
    dump_int(lo_mem_max);
    dump_int(rover);
    if ((eTeX_mode == 1)) {
        register integer for_end;
        k = 0 /*int_val */ ;
        for_end = 6 /*inter_char_val */ ;
        if (k <= for_end)
            do
                dump_int(sa_root[k]);
            while (k++ < for_end);
    }
    p = mem_bot;
    q = rover;
    x = 0;
    do {
        dump_things(mem[p], q + 2 - p);
        x = x + q + 2 - p;
        var_used = var_used + q - p;
        p = q + mem[q].hh.v.LH;
        q = mem[q + 1].hh.v.RH;
    } while (!(q == rover));
    var_used = var_used + lo_mem_max - p;
    dyn_used = mem_end + 1 - hi_mem_min;
    dump_things(mem[p], lo_mem_max + 1 - p);
    x = x + lo_mem_max + 1 - p;
    dump_int(hi_mem_min);
    dump_int(avail);
    dump_things(mem[hi_mem_min], mem_end + 1 - hi_mem_min);
    x = x + mem_end + 1 - hi_mem_min;
    p = avail;
    while (p != -268435455L) {

        dyn_used--;
        p = mem[p].hh.v.RH;
    }
    dump_int(var_used);
    dump_int(dyn_used);
    print_ln();
    print_int(x);
    print(S(_memory_locations_dumped__cu/*rrent usage is */));
    print_int(var_used);
    print_char(38 /*"&" */ );
    print_int(dyn_used);
    k = 1 /*active_base */ ;
    do {
        j = k;
        while (j < 8938739L /*int_base -1 */ ) {

            if ((eqtb[j].hh.v.RH == eqtb[j + 1].hh.v.RH) && (eqtb[j].hh.u.B0 == eqtb[j + 1].hh.u.B0)
                && (eqtb[j].hh.u.B1 == eqtb[j + 1].hh.u.B1))
                goto lab41;
            j++;
        }
        l = 8938740L /*int_base */ ;
        goto lab31;
 lab41:                        /*found1 */ j++;
        l = j;
        while (j < 8938739L /*int_base -1 */ ) {

            if ((eqtb[j].hh.v.RH != eqtb[j + 1].hh.v.RH) || (eqtb[j].hh.u.B0 != eqtb[j + 1].hh.u.B0)
                || (eqtb[j].hh.u.B1 != eqtb[j + 1].hh.u.B1))
                goto lab31;
            j++;
        }
 lab31:                        /*done1 */ dump_int(l - k);
        dump_things(eqtb[k], l - k);
        k = j + 1;
        dump_int(k - l);
    } while (!(k == 8938740L /*int_base *//*:1350 */ ));
    do {
        j = k;
        while (j < 10053470L /*eqtb_size */ ) {

            if (eqtb[j].cint == eqtb[j + 1].cint)
                goto lab42;
            j++;
        }
        l = 10053471L /*eqtb_size 1 */ ;
        goto lab32;
 lab42:                        /*found2 */ j++;
        l = j;
        while (j < 10053470L /*eqtb_size */ ) {

            if (eqtb[j].cint != eqtb[j + 1].cint)
                goto lab32;
            j++;
        }
 lab32:                        /*done2 */ dump_int(l - k);
        dump_things(eqtb[k], l - k);
        k = j + 1;
        dump_int(k - l);
    } while (!(k > 10053470L /*eqtb_size */ ));
    if (hash_high > 0)
        dump_things(eqtb[10053471L /*eqtb_size 1 */ ], hash_high);
    dump_int(par_loc);
    dump_int(write_loc);
    {
        register integer for_end;
        p = 0;
        for_end = 500 /*prim_size */ ;
        if (p <= for_end)
            do
                dump_hh(prim[p]);
            while (p++ < for_end);
    }
    {
        register integer for_end;
        p = 0;
        for_end = 500 /*prim_size */ ;
        if (p <= for_end)
            do
                dump_wd(prim_eqtb[p]);
            while (p++ < for_end);
    }
    dump_int(hash_used);
    cs_count = 2243225L /*frozen_control_sequence -1 */  - hash_used + hash_high;
    {
        register integer for_end;
        p = 2228226L /*hash_base */ ;
        for_end = hash_used;
        if (p <= for_end)
            do
                if (hash[p].v.RH != 0) {
                    dump_int(p);
                    dump_hh(hash[p]);
                    cs_count++;
                }
            while (p++ < for_end) ;
    }
    dump_things(hash[hash_used + 1], 2252238L /*undefined_control_sequence -1 */  - hash_used);
    if (hash_high > 0)
        dump_things(hash[10053471L /*eqtb_size 1 */ ], hash_high);
    dump_int(cs_count);
    print_ln();
    print_int(cs_count);
    print(S(_multiletter_control_sequenc/*es*/));
    dump_int(fmem_ptr);
    dump_things(font_info[0], fmem_ptr);
    dump_int(font_ptr);
    {
        dump_things(font_check[0 /*font_base */ ], font_ptr + 1);
        dump_things(font_size[0 /*font_base */ ], font_ptr + 1);
        dump_things(font_dsize[0 /*font_base */ ], font_ptr + 1);
        dump_things(font_params[0 /*font_base */ ], font_ptr + 1);
        dump_things(hyphen_char[0 /*font_base */ ], font_ptr + 1);
        dump_things(skew_char[0 /*font_base */ ], font_ptr + 1);
        dump_things(font_name[0 /*font_base */ ], font_ptr + 1);
        dump_things(font_area[0 /*font_base */ ], font_ptr + 1);
        dump_things(font_bc[0 /*font_base */ ], font_ptr + 1);
        dump_things(font_ec[0 /*font_base */ ], font_ptr + 1);
        dump_things(char_base[0 /*font_base */ ], font_ptr + 1);
        dump_things(width_base[0 /*font_base */ ], font_ptr + 1);
        dump_things(height_base[0 /*font_base */ ], font_ptr + 1);
        dump_things(depth_base[0 /*font_base */ ], font_ptr + 1);
        dump_things(italic_base[0 /*font_base */ ], font_ptr + 1);
        dump_things(lig_kern_base[0 /*font_base */ ], font_ptr + 1);
        dump_things(kern_base[0 /*font_base */ ], font_ptr + 1);
        dump_things(exten_base[0 /*font_base */ ], font_ptr + 1);
        dump_things(param_base[0 /*font_base */ ], font_ptr + 1);
        dump_things(font_glue[0 /*font_base */ ], font_ptr + 1);
        dump_things(bchar_label[0 /*font_base */ ], font_ptr + 1);
        dump_things(font_bchar[0 /*font_base */ ], font_ptr + 1);
        dump_things(font_false_bchar[0 /*font_base */ ], font_ptr + 1);
        {
            register integer for_end;
            k = 0 /*font_base */ ;
            for_end = font_ptr;
            if (k <= for_end)
                do {
                    print_nl(S(_font));
                    print_esc(hash[2243238L /*font_id_base */  + k].v.RH);
                    print_char(61 /*"=" */ );
                    if (((font_area[k] == 65535L /*aat_font_flag */ ) || (font_area[k] == 65534L /*otgr_font_flag */ ))
                        || (font_mapping[k] != 0)) {
                        print_file_name(font_name[k], S(), S());
                        {
                            if (interaction == 3 /*error_stop_mode */ ) ;
                            if (file_line_error_style_p)
                                print_file_line();
                            else
                                print_nl(S(__/*"! "*/));
                            print(S(Can_t__dump_a_format_with_na/*tive fonts or font-mappings*/));
                        }
                        {
                            help_ptr = 3;
                            help_line[2] = S(You_really__really_don_t_wan/*t to do this.*/);
                            help_line[1] = S(It_won_t_work__and_only_conf/*uses me.*/);
                            help_line[0] = S(_Load_them_at_runtime__not_a/*s part of the format file.)*/);
                        }
                        error();
                    } else
                        print_file_name(font_name[k], font_area[k], S());
                    if (font_size[k] != font_dsize[k]) {
                        print(S(_at_));
                        print_scaled(font_size[k]);
                        print(S(pt));
                    }
                }
                while (k++ < for_end);
        }
    }
    print_ln();
    print_int(fmem_ptr - 7);
    print(S(_words_of_font_info_for_));
    print_int(font_ptr - 0);
    if (font_ptr != 1 /*font_base 1 */ )
        print(S(_preloaded_fonts));
    else
        print(S(_preloaded_font));
    dump_int(hyph_count);
    if (hyph_next <= 607 /*hyph_prime */ )
        hyph_next = hyph_size;
    dump_int(hyph_next);
    {
        register integer for_end;
        k = 0;
        for_end = hyph_size;
        if (k <= for_end)
            do
                if (hyph_word[k] != 0) {
                    dump_int(k + 65536L * hyph_link[k]);
                    dump_int(hyph_word[k]);
                    dump_int(hyph_list[k]);
                }
            while (k++ < for_end) ;
    }
    print_ln();
    print_int(hyph_count);
    if (hyph_count != 1)
        print(S(_hyphenation_exceptions));
    else
        print(S(_hyphenation_exception));
    if (trie_not_ready)
        init_trie();
    dump_int(trie_max);
    dump_int(hyph_start);
    dump_things(trie_trl[0], trie_max + 1);
    dump_things(trie_tro[0], trie_max + 1);
    dump_things(trie_trc[0], trie_max + 1);
    dump_int(max_hyph_char);
    dump_int(trie_op_ptr);
    dump_things(hyf_distance[1], trie_op_ptr);
    dump_things(hyf_num[1], trie_op_ptr);
    dump_things(hyf_next[1], trie_op_ptr);
    print_nl(S(Hyphenation_trie_of_length_));
    print_int(trie_max);
    print(S(_has_));
    print_int(trie_op_ptr);
    if (trie_op_ptr != 1)
        print(S(_ops));
    else
        print(S(_op));
    print(S(_out_of_));
    print_int(trie_op_size);
    {
        register integer for_end;
        k = 255 /*biggest_lang */ ;
        for_end = 0;
        if (k >= for_end)
            do
                if (trie_used[k] > 0) {
                    print_nl(S(___Z12/*"  "*/));
                    print_int(trie_used[k]);
                    print(S(_for_language_));
                    print_int(k);
                    dump_int(k);
                    dump_int(trie_used[k]);
                }
            while (k-- > for_end) ;
    }
    dump_int(interaction);
    dump_int(format_ident);
    dump_int(69069L);
    eqtb[8938771L /*int_base 31 */ ].cint = 0 /*:1361 */ ;
    ttstub_output_close (fmt_out);
}


static void
pack_buffered_name(small_number n, integer a, integer b)
{
    integer k;
    UTF16_code c;
    integer j;

    if (n + b - a + 5 > INTEGER_MAX)
        b = a + INTEGER_MAX - n - 5;

    if (name_of_file)
        free(name_of_file);
    name_of_file = xmalloc_array(UTF8_code, n + (b - a + 1) + 5);

    k = 0;

    for (j = 1; j <= n; j++) {
	/* This junk is append_to_name(), inlined, and with UTF-8 decoding, I
	 * think. */
	c = TEX_format_default[j];
	k++;
	if (k <= INTEGER_MAX) {
	    if (c < 128) {
		name_of_file[k] = c;
	    } else if (c < 2048) {
		name_of_file[k++] = 192 + c / 64;
		name_of_file[k] = 128 + c % 64;
	    } else {
		name_of_file[k++] = 224 + c / 4096;
		name_of_file[k++] = 128 + (c % 4096) / 64;
		name_of_file[k] = 128 + (c % 4096) % 64;
	    }
	}
    }

    for (j = a; j <= b; j++) {
	c = buffer[j];
	k++;
	if (k <= INTEGER_MAX) {
	    if (c < 128) {
		name_of_file[k] = c;
	    } else if (c < 2048) {
		name_of_file[k++] = 192 + c / 64;
		name_of_file[k] = 128 + c % 64;
	    } else {
		name_of_file[k++] = 224 + c / 4096;
		name_of_file[k++] = 128 + (c % 4096) / 64;
		name_of_file[k] = 128 + (c % 4096) % 64;
	    }
	}
    }

    for (j = format_default_length - 3; j <= format_default_length; j++) {
	c = TEX_format_default[j];
	k++;
	if (k <= INTEGER_MAX) {
	    if (c < 128) {
		name_of_file[k] = c;
	    } else if (c < 2048) {
		name_of_file[k++] = 192 + c / 64;
		name_of_file[k] = 128 + c % 64;
	    } else {
		name_of_file[k++] = 224 + c / 4096;
		name_of_file[k++] = 128 + (c % 4096) / 64;
		name_of_file[k] = 128 + (c % 4096) % 64;
	    }
	}
    }

    if (k <= INTEGER_MAX)
        name_length = k;
    else
        name_length = INTEGER_MAX;

    name_of_file[name_length + 1] = 0;
}


static boolean
load_fmt_file(void)
{
    memory_word *mem = zmem, *eqtb = zeqtb;
    integer j, k;
    int32_t p, q;
    integer x;
    char *format_engine;
    rust_input_handle_t fmt_in;

    j = cur_input.loc;

    /* This is where a first line starting with "&" used to
     * trigger code that would change the format file. */

    pack_buffered_name(format_default_length - 4, 1, 0);

    fmt_in = ttstub_input_open(name_of_file + 1, kpse_fmt_format, 1);
    if (fmt_in == NULL)
	_tt_abort ("cannot open the format file \"%s\"", TEX_format_default + 1);

lab40: /* found */
    cur_input.loc = j;

    if (in_initex_mode) {
        free(font_info);
        free(str_pool);
        free(str_start);
        free(yhash);
        free(zeqtb);
        free(yzmem);
    }

    undump_int(x);
    if (x != 1462916184L)
        goto bad_fmt;
    undump_int(x);
    if ((x < 0) || (x > 256))
        goto bad_fmt;
    format_engine = xmalloc_array(char, x);
    undump_things(format_engine[0], x);
    format_engine[x - 1] = 0;
    if (strcmp(engine_name, (string) format_engine)) {
        fprintf(stdout, "---! %s was written by %s\n", (string) (name_of_file + 1), format_engine);
        free(format_engine);
        goto bad_fmt;
    }
    free(format_engine);
    undump_int(x);
    if (x != 457477274L) {
        fprintf(stdout, "---! %s doesn't match xetex.pool\n", (string) (name_of_file + 1));
        goto bad_fmt;
    }
    undump_int(x);
    if (x != 1073741823L)
        goto bad_fmt;
    undump_int(hash_high);
    if ((hash_high < 0) || (hash_high > sup_hash_extra))
        goto bad_fmt;
    if (hash_extra < hash_high)
        hash_extra = hash_high;
    eqtb_top = 10053470L /*eqtb_size */  + hash_extra;
    if (hash_extra == 0)
        hash_top = 2252239L /*undefined_control_sequence */ ;
    else
        hash_top = eqtb_top;
    yhash = xmalloc_array(two_halves, 1 + hash_top - hash_offset);
    hash = yhash - hash_offset;
    hash[2228226L /*hash_base */ ].v.LH = 0;
    hash[2228226L /*hash_base */ ].v.RH = 0;
    {
        register integer for_end;
        x = 2228227L /*hash_base 1 */ ;
        for_end = hash_top;
        if (x <= for_end)
            do
                hash[x] = hash[2228226L /*hash_base */ ];
            while (x++ < for_end);
    }
    zeqtb = xmalloc_array(memory_word, eqtb_top + 1);
    eqtb = zeqtb;
    eqtb[2252239L /*undefined_control_sequence */ ].hh.u.B0 = 103 /*undefined_cs */ ;
    eqtb[2252239L /*undefined_control_sequence */ ].hh.v.RH = -268435455L;
    eqtb[2252239L /*undefined_control_sequence */ ].hh.u.B1 = 0 /*level_zero */ ;
    {
        register integer for_end;
        x = 10053471L /*eqtb_size 1 */ ;
        for_end = eqtb_top;
        if (x <= for_end)
            do
                eqtb[x] = eqtb[2252239L /*undefined_control_sequence */ ];
            while (x++ < for_end);
    }
    {
        undump_int(x);
        if ((x < 0) || (x > 1))
            goto bad_fmt;
        else
            eTeX_mode = x;
    }
    if ((eTeX_mode == 1)) {
        max_reg_num = 32767;
        max_reg_help_line = S(A_register_number_must_be_be_Z1/*"A register number must be between 0 and 32767."*/);
    } else {

        max_reg_num = 255;
        max_reg_help_line = S(A_register_number_must_be_be/*tween 0 and 255.*/);
    }
    undump_int(x);
    if (x != mem_bot)
        goto bad_fmt;
    undump_int(mem_top);
    if (mem_bot + 1100 > mem_top)
        goto bad_fmt;
    cur_list.head = mem_top - 1;
    cur_list.tail = mem_top - 1;
    page_tail = mem_top - 2;
    mem_min = mem_bot - extra_mem_bot;
    mem_max = mem_top + extra_mem_top;
    yzmem = xmalloc_array(memory_word, mem_max - mem_min + 1);
    zmem = yzmem - mem_min;
    mem = zmem;
    undump_int(x);
    if (x != 10053470L /*eqtb_size */ )
        goto bad_fmt;
    undump_int(x);
    if (x != 8501 /*hash_prime */ )
        goto bad_fmt;
    undump_int(x);
    if (x != 607 /*hyph_prime */ )
        goto bad_fmt;
    undump_int(x);
    if (x != 1296847960L)
        goto bad_fmt;
    undump_int(x);
    if (x == 1)
        mltex_enabled_p = true;
    else if (x != 0)
        goto bad_fmt;
    {
        undump_int(x);
        if (x < 0)
            goto bad_fmt;
        if (x > sup_pool_size - pool_free)
            _tt_abort ("must increase string_pool_size");

        pool_ptr = x;
    }
    if (pool_size < pool_ptr + pool_free)
        pool_size = pool_ptr + pool_free;
    {
        undump_int(x);
        if (x < 0)
            goto bad_fmt;
        if (x > sup_max_strings - strings_free)
            _tt_abort ("must increase sup_strings");

        str_ptr = x;
    }
    if (max_strings < str_ptr + strings_free)
        max_strings = str_ptr + strings_free;
    str_start = xmalloc_array(pool_pointer, max_strings);
    undump_checked_things(0, pool_ptr, str_start[(65536L /*too_big_char */ ) - 65536L], str_ptr - 65535L);
    str_pool = xmalloc_array(packed_UTF16_code, pool_size);
    undump_things(str_pool[0], pool_ptr);
    init_str_ptr = str_ptr;
    init_pool_ptr = /*:1345 */ pool_ptr;
    {
        undump_int(x);
        if ((x < mem_bot + 1019) || (x > mem_top - 15))
            goto bad_fmt;
        else
            lo_mem_max = x;
    }
    {
        undump_int(x);
        if ((x < mem_bot + 20) || (x > lo_mem_max))
            goto bad_fmt;
        else
            rover = x;
    }
    if ((eTeX_mode == 1)) {
        register integer for_end;
        k = 0 /*int_val */ ;
        for_end = 6 /*inter_char_val */ ;
        if (k <= for_end)
            do {
                undump_int(x);
                if ((x < -268435455L) || (x > lo_mem_max))
                    goto bad_fmt;
                else
                    sa_root[k] = x;
            }
            while (k++ < for_end);
    }
    p = mem_bot;
    q = rover;
    do {
        undump_things(mem[p], q + 2 - p);
        p = q + mem[q].hh.v.LH;
        if ((p > lo_mem_max) || ((q >= mem[q + 1].hh.v.RH) && (mem[q + 1].hh.v.RH != rover)))
            goto bad_fmt;
        q = mem[q + 1].hh.v.RH;
    } while (!(q == rover));
    undump_things(mem[p], lo_mem_max + 1 - p);
    if (mem_min < mem_bot - 2) {
        p = mem[rover + 1].hh.v.LH;
        q = mem_min + 1;
        mem[mem_min].hh.v.RH = -268435455L;
        mem[mem_min].hh.v.LH = -268435455L;
        mem[p + 1].hh.v.RH = q;
        mem[rover + 1].hh.v.LH = q;
        mem[q + 1].hh.v.RH = rover;
        mem[q + 1].hh.v.LH = p;
        mem[q].hh.v.RH = 1073741823L;
        mem[q].hh.v.LH = mem_bot - q;
    }
    {
        undump_int(x);
        if ((x < lo_mem_max + 1) || (x > mem_top - 14))
            goto bad_fmt;
        else
            hi_mem_min = x;
    }
    {
        undump_int(x);
        if ((x < -268435455L) || (x > mem_top))
            goto bad_fmt;
        else
            avail = x;
    }
    mem_end = mem_top;
    undump_things(mem[hi_mem_min], mem_end + 1 - hi_mem_min);
    undump_int(var_used);
    undump_int(dyn_used);
    k = 1 /*active_base */ ;
    do {
        undump_int(x);
        if ((x < 1) || (k + x > 10053471L /*eqtb_size 1 */ ))
            goto bad_fmt;
        undump_things(eqtb[k], x);
        k = k + x;
        undump_int(x);
        if ((x < 0) || (k + x > 10053471L /*eqtb_size 1 */ ))
            goto bad_fmt;
        {
            register integer for_end;
            j = k;
            for_end = k + x - 1;
            if (j <= for_end)
                do
                    eqtb[j] = eqtb[k - 1];
                while (j++ < for_end);
        }
        k = k + x;
    } while (!(k > 10053470L /*eqtb_size */ ));
    if (hash_high > 0)
        undump_things(eqtb[10053471L /*eqtb_size 1 */ ], hash_high);
    {
        undump_int(x);
        if ((x < 2228226L /*hash_base */ ) || (x > hash_top))
            goto bad_fmt;
        else
            par_loc = x;
    }
    par_token = 33554431L /*cs_token_flag */  + par_loc;
    {
        undump_int(x);
        if ((x < 2228226L /*hash_base */ ) || (x > hash_top))
            goto bad_fmt;
        else
            write_loc = x;
    }
    {
        register integer for_end;
        p = 0;
        for_end = 500 /*prim_size */ ;
        if (p <= for_end)
            do
                undump_hh(prim[p]);
            while (p++ < for_end);
    }
    {
        register integer for_end;
        p = 0;
        for_end = 500 /*prim_size */ ;
        if (p <= for_end)
            do
                undump_wd(prim_eqtb[p]);
            while (p++ < for_end);
    }
    {
        undump_int(x);
        if ((x < 2228226L /*hash_base */ ) || (x > 2243226L /*frozen_control_sequence */ ))
            goto bad_fmt;
        else
            hash_used = x;
    }
    p = 2228225L /*hash_base -1 */ ;
    do {
        {
            undump_int(x);
            if ((x < p + 1) || (x > hash_used))
                goto bad_fmt;
            else
                p = x;
        }
        undump_hh(hash[p]);
    } while (!(p == hash_used));
    undump_things(hash[hash_used + 1], 2252238L /*undefined_control_sequence -1 */  - hash_used);
    if (hash_high > 0) {
        undump_things(hash[10053471L /*eqtb_size 1 */ ], hash_high);
    }
    undump_int(cs_count);
    {
        undump_int(x);
        if (x < 7)
            goto bad_fmt;
        if (x > sup_font_mem_size)
            _tt_abort ("must increase font_mem_size");

        fmem_ptr = x;
    }
    if (fmem_ptr > font_mem_size)
        font_mem_size = fmem_ptr;
    font_info = xmalloc_array(fmemory_word, font_mem_size);
    undump_things(font_info[0], fmem_ptr);
    {
        undump_int(x);
        if (x < 0 /*font_base */ )
            goto bad_fmt;
        if (x > 9000 /*font_base 9000 */ )
            _tt_abort ("must increase font_max");

        font_ptr = x;
    }
    {
        font_mapping = xmalloc_array(void *, font_max);
        font_layout_engine = xmalloc_array(void *, font_max);
        font_flags = xmalloc_array(char, font_max);
        font_letter_space = xmalloc_array(scaled, font_max);
        font_check = xmalloc_array(four_quarters, font_max);
        font_size = xmalloc_array(scaled, font_max);
        font_dsize = xmalloc_array(scaled, font_max);
        font_params = xmalloc_array(font_index, font_max);
        font_name = xmalloc_array(str_number, font_max);
        font_area = xmalloc_array(str_number, font_max);
        font_bc = xmalloc_array(UTF16_code, font_max);
        font_ec = xmalloc_array(UTF16_code, font_max);
        font_glue = xmalloc_array(int32_t, font_max);
        hyphen_char = xmalloc_array(integer, font_max);
        skew_char = xmalloc_array(integer, font_max);
        bchar_label = xmalloc_array(font_index, font_max);
        font_bchar = xmalloc_array(nine_bits, font_max);
        font_false_bchar = xmalloc_array(nine_bits, font_max);
        char_base = xmalloc_array(integer, font_max);
        width_base = xmalloc_array(integer, font_max);
        height_base = xmalloc_array(integer, font_max);
        depth_base = xmalloc_array(integer, font_max);
        italic_base = xmalloc_array(integer, font_max);
        lig_kern_base = xmalloc_array(integer, font_max);
        kern_base = xmalloc_array(integer, font_max);
        exten_base = xmalloc_array(integer, font_max);
        param_base = xmalloc_array(integer, font_max);
        {
            register integer for_end;
            k = 0 /*font_base */ ;
            for_end = font_ptr;
            if (k <= for_end)
                do
                    font_mapping[k] = 0;
                while (k++ < for_end);
        }
        undump_things(font_check[0 /*font_base */ ], font_ptr + 1);
        undump_things(font_size[0 /*font_base */ ], font_ptr + 1);
        undump_things(font_dsize[0 /*font_base */ ], font_ptr + 1);
        undump_checked_things(-268435455L, 1073741823L, font_params[0 /*font_base */ ], font_ptr + 1);
        undump_things(hyphen_char[0 /*font_base */ ], font_ptr + 1);
        undump_things(skew_char[0 /*font_base */ ], font_ptr + 1);
        undump_upper_check_things(str_ptr, font_name[0 /*font_base */ ], font_ptr + 1);
        undump_upper_check_things(str_ptr, font_area[0 /*font_base */ ], font_ptr + 1);
        undump_things(font_bc[0 /*font_base */ ], font_ptr + 1);
        undump_things(font_ec[0 /*font_base */ ], font_ptr + 1);
        undump_things(char_base[0 /*font_base */ ], font_ptr + 1);
        undump_things(width_base[0 /*font_base */ ], font_ptr + 1);
        undump_things(height_base[0 /*font_base */ ], font_ptr + 1);
        undump_things(depth_base[0 /*font_base */ ], font_ptr + 1);
        undump_things(italic_base[0 /*font_base */ ], font_ptr + 1);
        undump_things(lig_kern_base[0 /*font_base */ ], font_ptr + 1);
        undump_things(kern_base[0 /*font_base */ ], font_ptr + 1);
        undump_things(exten_base[0 /*font_base */ ], font_ptr + 1);
        undump_things(param_base[0 /*font_base */ ], font_ptr + 1);
        undump_checked_things(-268435455L, lo_mem_max, font_glue[0 /*font_base */ ], font_ptr + 1);
        undump_checked_things(0, fmem_ptr - 1, bchar_label[0 /*font_base */ ], font_ptr + 1);
        undump_checked_things(0, 65536L /*too_big_char */ , font_bchar[0 /*font_base */ ],
                              font_ptr + 1);
        undump_checked_things(0, 65536L /*too_big_char */ , font_false_bchar[0 /*font_base */ ],
                              font_ptr + 1);
    }
    {
        undump_int(x);
        if (x < 0)
            goto bad_fmt;
        if (x > hyph_size)
            _tt_abort ("must increase hyph_size");

        hyph_count = x;
    }
    {
        undump_int(x);
        if (x < 607 /*hyph_prime */ )
            goto bad_fmt;
        if (x > hyph_size)
            _tt_abort ("must increase hyph_size");

        hyph_next = x;
    }
    j = 0;
    {
        register integer for_end;
        k = 1;
        for_end = hyph_count;
        if (k <= for_end)
            do {
                undump_int(j);
                if (j < 0)
                    goto bad_fmt;
                if (j > 65535L) {
                    hyph_next = j / 65536L;
                    j = j - hyph_next * 65536L;
                } else
                    hyph_next = 0;
                if ((j >= hyph_size) || (hyph_next > hyph_size))
                    goto bad_fmt;
                hyph_link[j] = hyph_next;
                {
                    undump_int(x);
                    if ((x < 0) || (x > str_ptr))
                        goto bad_fmt;
                    else
                        hyph_word[j] = x;
                }
                {
                    undump_int(x);
                    if ((x < -268435455L) || (x > 1073741823L))
                        goto bad_fmt;
                    else
                        hyph_list[j] = x;
                }
            }
            while (k++ < for_end);
    }
    j++;
    if (j < 607 /*hyph_prime */ )
        j = 607 /*hyph_prime */ ;
    hyph_next = j;
    if (hyph_next >= hyph_size)
        hyph_next = 607 /*hyph_prime */ ;
    else if (hyph_next >= 607 /*hyph_prime */ )
        hyph_next++;
    {
        undump_int(x);
        if (x < 0)
            goto bad_fmt;
        if (x > trie_size)
	    _tt_abort ("must increase trie_size");

        j = x;
    }

    trie_max = j;
    {
        undump_int(x);
        if ((x < 0) || (x > j))
            goto bad_fmt;
        else
            hyph_start = x;
    }
    if (!trie_trl)
        trie_trl = xmalloc_array(trie_pointer, j + 1);
    undump_things(trie_trl[0], j + 1);
    if (!trie_tro)
        trie_tro = xmalloc_array(trie_pointer, j + 1);
    undump_things(trie_tro[0], j + 1);
    if (!trie_trc)
        trie_trc = xmalloc_array(uint16_t, j + 1);
    undump_things(trie_trc[0], j + 1);
    undump_int(max_hyph_char);
    {
        undump_int(x);
        if (x < 0)
            goto bad_fmt;
        if (x > trie_op_size)
	    _tt_abort ("must increase trie_op_size");

        j = x;
    }

    trie_op_ptr = j;

    undump_things(hyf_distance[1], j);
    undump_things(hyf_num[1], j);
    undump_upper_check_things(max_trie_op, hyf_next[1], j);

    {
        register integer for_end;
        k = 0;
        for_end = 255 /*biggest_lang */ ;
        if (k <= for_end)
            do
                trie_used[k] = 0;
            while (k++ < for_end);
    }

    k = 256 /*biggest_lang 1 */ ;
    while (j > 0) {

        {
            undump_int(x);
            if ((x < 0) || (x > k - 1))
                goto bad_fmt;
            else
                k = x;
        }
        {
            undump_int(x);
            if ((x < 1) || (x > j))
                goto bad_fmt;
            else
                x = x;
        }

        trie_used[k] = x;
        j = j - x;
        op_start[k] = j;
    }

    trie_not_ready = false;

    {
        undump_int(x);
        if ((x < 0 /*batch_mode */ ) || (x > 3 /*error_stop_mode */ ))
            goto bad_fmt;
        else
            interaction = x;
    }
    if (interaction_option != 4 /*unspecified_mode */ )
        interaction = interaction_option;
    {
        undump_int(x);
        if ((x < 0) || (x > str_ptr))
            goto bad_fmt;
        else
            format_ident = x;
    }
    undump_int(x);
    if (x != 69069L)
        goto bad_fmt;

    ttstub_input_close (fmt_in);
    return true;

bad_fmt:
    _tt_abort ("fatal format file error");
}

static void
final_cleanup(void)
{
    memory_word *mem = zmem;
    small_number c;

    c = cur_chr;
    if (job_name == 0)
        open_log_file();
    while (input_ptr > 0)
        if (cur_input.state == 0 /*token_list */ )
            end_token_list();
        else
            end_file_reading();
    while (open_parens > 0) {

        print(S(___Z19/*" )"*/));
        open_parens--;
    }
    if (cur_level > 1 /*level_one */ ) {
        print_nl(40 /*"(" */ );
        print_esc(S(end_occurred_));
        print(S(inside_a_group_at_level_));
        print_int(cur_level - 1);
        print_char(41 /*")" */ );
        if ((eTeX_mode == 1))
            show_save_groups();
    }
    while (cond_ptr != -268435455L) {

        print_nl(40 /*"(" */ );
        print_esc(S(end_occurred_));
        print(S(when_));
        print_cmd_chr(107 /*if_test */ , cur_if);
        if (if_line != 0) {
            print(S(_on_line_));
            print_int(if_line);
        }
        print(S(_was_incomplete_));
        if_line = mem[cond_ptr + 1].cint;
        cur_if = mem[cond_ptr].hh.u.B1;
        temp_ptr = cond_ptr;
        cond_ptr = mem[cond_ptr].hh.v.RH;
        free_node(temp_ptr, 2 /*if_node_size */ );
    }

    if (history != HISTORY_SPOTLESS) {
        if ((history == HISTORY_WARNING_ISSUED || (interaction < 3 /*error_stop_mode */ ))) {

            if (selector == SELECTOR_TERM_AND_LOG) {
                selector = SELECTOR_TERM_ONLY;
                print_nl(S(_see_the_transcript_file_for/* additional information)*/));
                selector = SELECTOR_TERM_AND_LOG;
            }
        }
    }
    if (c == 1) {
        if (in_initex_mode) {
            {
                register integer for_end;
                c = 0 /*top_mark_code */ ;
                for_end = 4 /*split_bot_mark_code */ ;
                if (c <= for_end)
                    do
                        if (cur_mark[c] != -268435455L)
                            delete_token_ref(cur_mark[c]);
                    while (c++ < for_end) ;
            }
            if (sa_root[7 /*mark_val */ ] != -268435455L) {

                if (do_marks(3, 0, sa_root[7 /*mark_val */ ]))
                    sa_root[7 /*mark_val */ ] = -268435455L;
            }
            {
                register integer for_end;
                c = 2 /*last_box_code */ ;
                for_end = 3 /*vsplit_code */ ;
                if (c <= for_end)
                    do
                        flush_node_list(disc_ptr[c]);
                    while (c++ < for_end);
            }
            if (last_glue != 1073741823L)
                delete_glue_ref(last_glue);
            store_fmt_file();
            return;
        }
        print_nl(S(__dump_is_performed_only_by_/*INITEX)*/));
        return;
    }
}


/* Engine initialization */

static UFILE stdin_ufile;

static boolean
init_terminal(string input_file_name)
{
    int k;
    unsigned char *ptr = (unsigned char *) input_file_name;
    UInt32 rval;

    stdin_ufile.handle = NULL;
    stdin_ufile.savedChar = -1;
    stdin_ufile.skipNextLF = 0;
    stdin_ufile.encodingMode = UTF8;
    stdin_ufile.conversionData = 0;
    input_file[0] = &stdin_ufile;

    /* Hacky stuff that sets us up to process the input file, including UTF8
     * interpretation. */

    buffer[first] = 0;
    k = first;

    while ((rval = *(ptr++)) != 0) {
	UInt16 extraBytes = bytesFromUTF8[rval];

	switch (extraBytes) { /* note: code falls through cases! */
	case 5: rval <<= 6; if (*ptr) rval += *(ptr++);
	case 4: rval <<= 6; if (*ptr) rval += *(ptr++);
	case 3: rval <<= 6; if (*ptr) rval += *(ptr++);
	case 2: rval <<= 6; if (*ptr) rval += *(ptr++);
	case 1: rval <<= 6; if (*ptr) rval += *(ptr++);
	case 0: ;
	}

	rval -= offsetsFromUTF8[extraBytes];
	buffer[k++] = rval;
    }

    buffer[k++] = ' ';

    /* Find the end of the buffer.  */
    for (last = first; buffer[last]; ++last)
	;

    /* Make `last' be one past the last non-blank character in `buffer'.  */
    /* ??? The test for '\r' should not be necessary.  */
    for (--last; last >= first
	     && ISBLANK (buffer[last]) && buffer[last] != '\r'; --last)
	;
    last++;

    /* TODO: we don't want/need special stdin handling, so the following code
     * should disappear */

    if (last > first) {
        cur_input.loc = first;
        while ((cur_input.loc < last) && (buffer[cur_input.loc] == ' '))
            cur_input.loc++;
        if (cur_input.loc < last)
            return true;
    }

    _tt_abort ("internal error TERMINPUT");
}


static void
initialize_more_variables(void)
{
    memory_word *mem = zmem, *eqtb = zeqtb;
    integer i, k;
    hyph_pointer z;

    doing_special = false;
    native_text_size = 128;
    native_text = xmalloc(native_text_size * sizeof(UTF16_code));

    if (interaction_option == 4 /*unspecified_mode */ )
        interaction = 3 /*error_stop_mode */ ;
    else
        interaction = interaction_option;

    deletions_allowed = true;
    set_box_allowed = true;
    error_count = 0;
    help_ptr = 0;
    use_err_help = false;

    nest_ptr = 0;
    max_nest_stack = 0;
    cur_list.mode = 1 /*vmode */ ;
    cur_list.head = mem_top - 1;
    cur_list.tail = mem_top - 1;
    cur_list.eTeX_aux = -268435455L;
    cur_list.aux.cint = -65536000L;
    cur_list.ml = 0;
    cur_list.pg = 0;
    shown_mode = 0;
    page_contents = 0 /*empty */ ;
    page_tail = mem_top - 2;
    last_glue = 1073741823L;
    last_penalty = 0;
    last_kern = 0;
    page_so_far[7] = 0;
    page_max_depth = 0;

    {
        register integer for_end;
        k = 8938740L /*int_base */ ;
        for_end = 10053470L /*eqtb_size */ ;
        if (k <= for_end)
            do
                xeq_level[k] = 1 /*level_one */ ;
            while (k++ < for_end);
    }

    no_new_control_sequence = true;
    prim[0].v.LH = 0;
    prim[0].v.RH = 0;

    {
        register integer for_end;
        k = 1;
        for_end = 500 /*prim_size */ ;
        if (k <= for_end)
            do
                prim[k] = prim[0];
            while (k++ < for_end);
    }

    prim_eqtb[0].hh.u.B1 = 0 /*level_zero */ ;
    prim_eqtb[0].hh.u.B0 = 103 /*undefined_cs */ ;
    prim_eqtb[0].hh.v.RH = -268435455L;

    {
        register integer for_end;
        k = 1;
        for_end = 500 /*prim_size */ ;
        if (k <= for_end)
            do
                prim_eqtb[k] = prim_eqtb[0];
            while (k++ < for_end);
    }

    save_ptr = 0;
    cur_level = 1 /*level_one */ ;
    cur_group = 0 /*bottom_level */ ;
    cur_boundary = 0;
    max_save_stack = 0;
    mag_set = 0;
    expand_depth_count = 0;
    is_in_csname = false;
    cur_mark[0 /*top_mark_code */ ] = -268435455L;
    cur_mark[1 /*first_mark_code */ ] = -268435455L;
    cur_mark[2 /*bot_mark_code */ ] = -268435455L;
    cur_mark[3 /*split_first_mark_code */ ] = -268435455L;
    cur_mark[4 /*split_bot_mark_code */ ] = -268435455L;
    cur_val = 0;
    cur_val_level = 0 /*int_val */ ;
    radix = 0;
    cur_order = 0 /*normal */ ;

    {
        register integer for_end;
        k = 0;
        for_end = 16;
        if (k <= for_end)
            do
                read_open[k] = 2 /*closed */ ;
            while (k++ < for_end);
    }

    cond_ptr = -268435455L;
    if_limit = 0 /*normal */ ;
    cur_if = 0;
    if_line = 0;
    null_character.u.B0 = 0;
    null_character.u.B1 = 0;
    null_character.u.B2 = 0;
    null_character.u.B3 = 0;
    total_pages = 0;
    max_v = 0;
    max_h = 0;
    max_push = 0;
    last_bop = -1;
    doing_leaders = false;
    dead_cycles = 0;
    cur_s = -1;
    half_buf = dvi_buf_size / 2;
    dvi_limit = dvi_buf_size;
    dvi_ptr = 0;
    dvi_offset = 0;
    dvi_gone = 0;
    down_ptr = -268435455L;
    right_ptr = -268435455L;
    adjust_tail = -268435455L;
    last_badness = 0;
    pre_adjust_tail = -268435455L;
    pack_begin_line = 0;
    empty.v.RH = 0 /*empty */ ;
    empty.v.LH = -268435455L;
    null_delimiter.u.B0 = 0;
    null_delimiter.u.B1 = 0;
    null_delimiter.u.B2 = 0;
    null_delimiter.u.B3 = 0;
    align_ptr = -268435455L;
    cur_align = -268435455L;
    cur_span = -268435455L;
    cur_loop = -268435455L;
    cur_head = -268435455L;
    cur_tail = -268435455L;
    cur_pre_head = -268435455L;
    cur_pre_tail = -268435455L;
    max_hyph_char = 256 /*too_big_lang */ ;

    {
        register integer for_end;
        z = 0;
        for_end = hyph_size;
        if (z <= for_end)
            do {
                hyph_word[z] = 0;
                hyph_list[z] = -268435455L;
                hyph_link[z] = 0;
            }
            while (z++ < for_end);
    }

    hyph_count = 0;
    hyph_next = 608 /*hyph_prime 1 */ ;
    if (hyph_next > hyph_size)
        hyph_next = 607 /*hyph_prime */ ;

    output_active = false;
    insert_penalties = 0;
    ligature_present = false;
    cancel_boundary = false;
    lft_hit = false;
    rt_hit = false;
    ins_disc = false;
    after_token = 0;
    long_help_seen = false;
    format_ident = 0;
    {
        register integer for_end;
        k = 0;
        for_end = 17;
        if (k <= for_end)
            do
                write_open[k] = false;
            while (k++ < for_end);
    }
    LR_ptr = -268435455L;
    LR_problems = 0;
    cur_dir = 0 /*left_to_right */ ;
    pseudo_files = -268435455L;
    sa_root[7 /*mark_val */ ] = -268435455L;
    sa_null.hh.v.LH = -268435455L;
    sa_null.hh.v.RH = -268435455L;
    sa_chain = -268435455L;
    sa_level = 0 /*level_zero */ ;
    disc_ptr[2 /*last_box_code */ ] = -268435455L;
    disc_ptr[3 /*vsplit_code */ ] = -268435455L;
    edit_name_start = 0;
    stop_at_space = true;
    mltex_enabled_p = false;

    if (in_initex_mode) {
        {
            register integer for_end;
            k = mem_bot + 1;
            for_end = mem_bot + 19;
            if (k <= for_end)
                do
                    mem[k].cint = 0;
                while (k++ < for_end);
        }
        k = mem_bot;
        while (k <= mem_bot + 19) {

            mem[k].hh.v.RH = -268435454L;
            mem[k].hh.u.B0 = 0 /*normal */ ;
            mem[k].hh.u.B1 = 0 /*normal */ ;
            k = k + 4;
        }
        mem[mem_bot + 6].cint = 65536L;
        mem[mem_bot + 4].hh.u.B0 = 1 /*fil */ ;
        mem[mem_bot + 10].cint = 65536L;
        mem[mem_bot + 8].hh.u.B0 = 2 /*fill */ ;
        mem[mem_bot + 14].cint = 65536L;
        mem[mem_bot + 12].hh.u.B0 = 1 /*fil */ ;
        mem[mem_bot + 15].cint = 65536L;
        mem[mem_bot + 12].hh.u.B1 = 1 /*fil */ ;
        mem[mem_bot + 18].cint = -65536L;
        mem[mem_bot + 16].hh.u.B0 = 1 /*fil */ ;
        rover = mem_bot + 20;
        mem[rover].hh.v.RH = 1073741823L;
        mem[rover].hh.v.LH = 1000;
        mem[rover + 1].hh.v.LH = rover;
        mem[rover + 1].hh.v.RH = rover;
        lo_mem_max = rover + 1000;
        mem[lo_mem_max].hh.v.RH = -268435455L;
        mem[lo_mem_max].hh.v.LH = -268435455L;
        {
            register integer for_end;
            k = mem_top - 14;
            for_end = mem_top;
            if (k <= for_end)
                do
                    mem[k] = mem[lo_mem_max];
                while (k++ < for_end);
        }
        mem[mem_top - 10].hh.v.LH = 35797662L /*cs_token_flag 2243231 */ ;
        mem[mem_top - 9].hh.v.RH = UINT16_MAX + 1;
        mem[mem_top - 9].hh.v.LH = -268435455L;
        mem[mem_top - 7].hh.u.B0 = 1 /*hyphenated */ ;
        mem[mem_top - 6].hh.v.LH = 1073741823L;
        mem[mem_top - 7].hh.u.B1 = 0;
        mem[mem_top].hh.u.B1 = 255;
        mem[mem_top].hh.u.B0 = 1 /*split_up */ ;
        mem[mem_top].hh.v.RH = mem_top;
        mem[mem_top - 2].hh.u.B0 = 10 /*glue_node */ ;
        mem[mem_top - 2].hh.u.B1 = 0 /*normal */ ;
        avail = -268435455L;
        mem_end = mem_top;
        hi_mem_min = mem_top - 14;
        var_used = mem_bot + 20 - mem_bot;
        dyn_used = 15 /*hi_mem_stat_usage */ ;
        eqtb[2252239L /*undefined_control_sequence */ ].hh.u.B0 = 103 /*undefined_cs */ ;
        eqtb[2252239L /*undefined_control_sequence */ ].hh.v.RH = -268435455L;
        eqtb[2252239L /*undefined_control_sequence */ ].hh.u.B1 = 0 /*level_zero */ ;
        {
            register integer for_end;
            k = 1 /*active_base */ ;
            for_end = eqtb_top;
            if (k <= for_end)
                do
                    eqtb[k] = eqtb[2252239L /*undefined_control_sequence */ ];
                while (k++ < for_end);
        }
        eqtb[2252240L /*glue_base */ ].hh.v.RH = mem_bot;
        eqtb[2252240L /*glue_base */ ].hh.u.B1 = 1 /*level_one */ ;
        eqtb[2252240L /*glue_base */ ].hh.u.B0 = 119 /*glue_ref */ ;
        {
            register integer for_end;
            k = 2252241L /*glue_base 1 */ ;
            for_end = 2252770L /*local_base -1 */ ;
            if (k <= for_end)
                do
                    eqtb[k] = eqtb[2252240L /*glue_base */ ];
                while (k++ < for_end);
        }
        mem[mem_bot].hh.v.RH = mem[mem_bot].hh.v.RH + 531;
        eqtb[2252771L /*par_shape_loc */ ].hh.v.RH = -268435455L;
        eqtb[2252771L /*par_shape_loc */ ].hh.u.B0 = 120 /*shape_ref */ ;
        eqtb[2252771L /*par_shape_loc */ ].hh.u.B1 = 1 /*level_one */ ;
        {
            register integer for_end;
            k = 2253039L /*etex_pen_base */ ;
            for_end = 2253042L /*etex_pens -1 */ ;
            if (k <= for_end)
                do
                    eqtb[k] = eqtb[2252771L /*par_shape_loc */ ];
                while (k++ < for_end);
        }
        {
            register integer for_end;
            k = 2252772L /*output_routine_loc */ ;
            for_end = 2253038L /*toks_base 256 -1 */ ;
            if (k <= for_end)
                do
                    eqtb[k] = eqtb[2252239L /*undefined_control_sequence */ ];
                while (k++ < for_end);
        }
        eqtb[2253043L /*box_base 0 */ ].hh.v.RH = -268435455L;
        eqtb[2253043L /*box_base */ ].hh.u.B0 = 121 /*box_ref */ ;
        eqtb[2253043L /*box_base */ ].hh.u.B1 = 1 /*level_one */ ;
        {
            register integer for_end;
            k = 2253044L /*box_base 1 */ ;
            for_end = 2253298L /*box_base 256 -1 */ ;
            if (k <= for_end)
                do
                    eqtb[k] = eqtb[2253043L /*box_base */ ];
                while (k++ < for_end);
        }
        eqtb[2253299L /*cur_font_loc */ ].hh.v.RH = 0 /*font_base */ ;
        eqtb[2253299L /*cur_font_loc */ ].hh.u.B0 = 122 /*data */ ;
        eqtb[2253299L /*cur_font_loc */ ].hh.u.B1 = 1 /*level_one */ ;
        {
            register integer for_end;
            k = 2253300L /*math_font_base */ ;
            for_end = 2254067L /*math_font_base 768 -1 */ ;
            if (k <= for_end)
                do
                    eqtb[k] = eqtb[2253299L /*cur_font_loc */ ];
                while (k++ < for_end);
        }
        eqtb[2254068L /*cat_code_base */ ].hh.v.RH = 0;
        eqtb[2254068L /*cat_code_base */ ].hh.u.B0 = 122 /*data */ ;
        eqtb[2254068L /*cat_code_base */ ].hh.u.B1 = 1 /*level_one */ ;
        {
            register integer for_end;
            k = 2254069L /*cat_code_base 1 */ ;
            for_end = 8938739L /*int_base -1 */ ;
            if (k <= for_end)
                do
                    eqtb[k] = eqtb[2254068L /*cat_code_base */ ];
                while (k++ < for_end);
        }
        {
            register integer for_end;
            k = 0;
            for_end = 1114111L /*number_usvs -1 */ ;
            if (k <= for_end)
                do {
                    eqtb[2254068L /*cat_code_base */  + k].hh.v.RH = 12 /*other_char */ ;
                    eqtb[6710516L /*math_code_base */  + k].hh.v.RH = k;
                    eqtb[5596404L /*sf_code_base */  + k].hh.v.RH = 1000;
                }
                while (k++ < for_end);
        }
        eqtb[2254081L /*cat_code_base 13 */ ].hh.v.RH = 5 /*car_ret */ ;
        eqtb[2254100L /*cat_code_base 32 */ ].hh.v.RH = 10 /*spacer */ ;
        eqtb[2254160L /*cat_code_base 92 */ ].hh.v.RH = 0 /*escape */ ;
        eqtb[2254105L /*cat_code_base 37 */ ].hh.v.RH = 14 /*comment */ ;
        eqtb[2254195L /*cat_code_base 127 */ ].hh.v.RH = 15 /*invalid_char */ ;
        eqtb[2254068L /*cat_code_base 0 */ ].hh.v.RH = 9 /*ignore */ ;
        {
            register integer for_end;
            k = 48 /*"0" */ ;
            for_end = 57 /*"9" */ ;
            if (k <= for_end)
                do
                    eqtb[6710516L /*math_code_base */  + k].hh.v.RH = k + set_class(7 /*var_fam_class */ );
                while (k++ < for_end);
        }
        {
            register integer for_end;
            k = 65 /*"A" */ ;
            for_end = 90 /*"Z" */ ;
            if (k <= for_end)
                do {
                    eqtb[2254068L /*cat_code_base */  + k].hh.v.RH = 11 /*letter */ ;
                    eqtb[2254068L /*cat_code_base */  + k + 32].hh.v.RH = 11 /*letter */ ;
                    eqtb[6710516L /*math_code_base */  + k].hh.v.RH =
                        k + set_family(1) + set_class(7 /*var_fam_class */ );
                    eqtb[6710516L /*math_code_base */  + k + 32].hh.v.RH =
                        k + 32 + set_family(1) + set_class(7 /*var_fam_class */ );
                    eqtb[3368180L /*lc_code_base */  + k].hh.v.RH = k + 32;
                    eqtb[3368180L /*lc_code_base */  + k + 32].hh.v.RH = k + 32;
                    eqtb[4482292L /*uc_code_base */  + k].hh.v.RH = k;
                    eqtb[4482292L /*uc_code_base */  + k + 32].hh.v.RH = k;
                    eqtb[5596404L /*sf_code_base */  + k].hh.v.RH = 999;
                }
                while (k++ < for_end);
        }
        {
            register integer for_end;
            k = 8938740L /*int_base */ ;
            for_end = 8939079L /*del_code_base -1 */ ;
            if (k <= for_end)
                do
                    eqtb[k].cint = 0;
                while (k++ < for_end);
        }
        eqtb[8938795L /*int_base 55 */ ].cint = 256;
        eqtb[8938796L /*int_base 56 */ ].cint = -1;
        eqtb[8938757L /*int_base 17 */ ].cint = 1000;
        eqtb[8938741L /*int_base 1 */ ].cint = 10000;
        eqtb[8938781L /*int_base 41 */ ].cint = 1;
        eqtb[8938780L /*int_base 40 */ ].cint = 25;
        eqtb[8938785L /*int_base 45 */ ].cint = 92 /*"\" */ ;
        eqtb[8938788L /*int_base 48 */ ].cint = 13 /*carriage_return */ ;
        {
            register integer for_end;
            k = 0;
            for_end = 65535L /*number_chars -1 */ ;
            if (k <= for_end)
                do
                    eqtb[8939080L /*del_code_base */  + k].cint = -1;
                while (k++ < for_end);
        }
        eqtb[8939126L /*del_code_base 46 */ ].cint = 0;
        {
            register integer for_end;
            k = 10053192L /*dimen_base */ ;
            for_end = 10053470L /*eqtb_size */ ;
            if (k <= for_end)
                do
                    eqtb[k].cint = 0;
                while (k++ < for_end);
        }
        prim_used = 500 /*prim_size */ ;
        hash_used = 2243226L /*frozen_control_sequence */ ;
        hash_high = 0;
        cs_count = 0;
        eqtb[2243235L /*frozen_dont_expand */ ].hh.u.B0 = 118 /*dont_expand */ ;
        hash[2243235L /*frozen_dont_expand */ ].v.RH = S(notexpanded_);
        eqtb[2243237L /*frozen_primitive */ ].hh.u.B0 = 39 /*ignore_spaces */ ;
        eqtb[2243237L /*frozen_primitive */ ].hh.v.RH = 1;
        eqtb[2243237L /*frozen_primitive */ ].hh.u.B1 = 1 /*level_one */ ;
        hash[2243237L /*frozen_primitive */ ].v.RH = S(primitive);
        {
            register integer for_end;
            k = -(integer) trie_op_size;
            for_end = trie_op_size;
            if (k <= for_end)
                do
                    trie_op_hash[k] = 0;
                while (k++ < for_end);
        }
        {
            register integer for_end;
            k = 0;
            for_end = 255 /*biggest_lang */ ;
            if (k <= for_end)
                do
                    trie_used[k] = min_trie_op;
                while (k++ < for_end);
        }
        max_op_used = min_trie_op;
        trie_op_ptr = 0;
        trie_not_ready = true;
        hash[2243226L /*frozen_protection */ ].v.RH = S(inaccessible);
        if (in_initex_mode)
            format_ident = S(__INITEX_);
        hash[2243234L /*end_write */ ].v.RH = S(endwrite);
        eqtb[2243234L /*end_write */ ].hh.u.B1 = 1 /*level_one */ ;
        eqtb[2243234L /*end_write */ ].hh.u.B0 = 115 /*outer_call */ ;
        eqtb[2243234L /*end_write */ ].hh.v.RH = -268435455L;
        eTeX_mode = 0;
        max_reg_num = 255;
        max_reg_help_line = S(A_register_number_must_be_be/*tween 0 and 255.*/);
        {
            register integer for_end;
            i = 0 /*int_val */ ;
            for_end = 6 /*inter_char_val */ ;
            if (i <= for_end)
                do
                    sa_root[i] = -268435455L;
                while (i++ < for_end);
        }
        eqtb[8938822L /*eTeX_state_base 11 */ ].cint = 63;
    }

    synctexoffset = 8938823L /*int_base 83 */ ;
}


/*:1370*//*1371: */
static void
initialize_primitives(void)
{
    memory_word *eqtb = zeqtb;

    no_new_control_sequence = false;
    first = 0;

    primitive(65671L /*"lineskip"*/, 76 /*assign_glue*/, 2252240L /*glue_base 0*/);
    primitive(65672L /*"baselineskip"*/, 76 /*assign_glue*/, 2252241L /*glue_base 1*/);
    primitive(65673L /*"parskip"*/, 76 /*assign_glue*/, 2252242L /*glue_base 2*/);
    primitive(65674L /*"abovedisplayskip"*/, 76 /*assign_glue*/, 2252243L /*glue_base 3*/);
    primitive(65675L /*"belowdisplayskip"*/, 76 /*assign_glue*/, 2252244L /*glue_base 4*/);
    primitive(65676L /*"abovedisplayshortskip"*/, 76 /*assign_glue*/, 2252245L /*glue_base 5*/);
    primitive(65677L /*"belowdisplayshortskip"*/, 76 /*assign_glue*/, 2252246L /*glue_base 6*/);
    primitive(65678L /*"leftskip"*/, 76 /*assign_glue*/, 2252247L /*glue_base 7*/);
    primitive(65679L /*"rightskip"*/, 76 /*assign_glue*/, 2252248L /*glue_base 8*/);
    primitive(65680L /*"topskip"*/, 76 /*assign_glue*/, 2252249L /*glue_base 9*/);
    primitive(65681L /*"splittopskip"*/, 76 /*assign_glue*/, 2252250L /*glue_base 10*/);
    primitive(65682L /*"tabskip"*/, 76 /*assign_glue*/, 2252251L /*glue_base 11*/);
    primitive(65683L /*"spaceskip"*/, 76 /*assign_glue*/, 2252252L /*glue_base 12*/);
    primitive(65684L /*"xspaceskip"*/, 76 /*assign_glue*/, 2252253L /*glue_base 13*/);
    primitive(65685L /*"parfillskip"*/, 76 /*assign_glue*/, 2252254L /*glue_base 14*/);
    primitive(65686L /*"XeTeXlinebreakskip"*/, 76 /*assign_glue*/, 2252255L /*glue_base 15*/);
    primitive(65687L /*"thinmuskip"*/, 77 /*assign_mu_glue*/, 2252256L /*glue_base 16*/);
    primitive(65688L /*"medmuskip"*/, 77 /*assign_mu_glue*/, 2252257L /*glue_base 17*/);
    primitive(65689L /*"thickmuskip"*/, 77 /*assign_mu_glue*/, 2252258L /*glue_base 18*/);
    primitive(65694L /*"output"*/, 73 /*assign_toks*/, 2252772L /*output_routine_loc*/);
    primitive(65695L /*"everypar"*/, 73 /*assign_toks*/, 2252773L /*every_par_loc*/);
    primitive(65696L /*"everymath"*/, 73 /*assign_toks*/, 2252774L /*every_math_loc*/);
    primitive(65697L /*"everydisplay"*/, 73 /*assign_toks*/, 2252775L /*every_display_loc*/);
    primitive(65698L /*"everyhbox"*/, 73 /*assign_toks*/, 2252776L /*every_hbox_loc*/);
    primitive(65699L /*"everyvbox"*/, 73 /*assign_toks*/, 2252777L /*every_vbox_loc*/);
    primitive(65700L /*"everyjob"*/, 73 /*assign_toks*/, 2252778L /*every_job_loc*/);
    primitive(65701L /*"everycr"*/, 73 /*assign_toks*/, 2252779L /*every_cr_loc*/);
    primitive(65702L /*"errhelp"*/, 73 /*assign_toks*/, 2252780L /*err_help_loc*/);
    primitive(65716L /*"pretolerance"*/, 74 /*assign_int*/, 8938740L /*int_base 0*/);
    primitive(65717L /*"tolerance"*/, 74 /*assign_int*/, 8938741L /*int_base 1*/);
    primitive(65718L /*"linepenalty"*/, 74 /*assign_int*/, 8938742L /*int_base 2*/);
    primitive(65719L /*"hyphenpenalty"*/, 74 /*assign_int*/, 8938743L /*int_base 3*/);
    primitive(65720L /*"exhyphenpenalty"*/, 74 /*assign_int*/, 8938744L /*int_base 4*/);
    primitive(65721L /*"clubpenalty"*/, 74 /*assign_int*/, 8938745L /*int_base 5*/);
    primitive(65722L /*"widowpenalty"*/, 74 /*assign_int*/, 8938746L /*int_base 6*/);
    primitive(65723L /*"displaywidowpenalty"*/, 74 /*assign_int*/, 8938747L /*int_base 7*/);
    primitive(65724L /*"brokenpenalty"*/, 74 /*assign_int*/, 8938748L /*int_base 8*/);
    primitive(65725L /*"binoppenalty"*/, 74 /*assign_int*/, 8938749L /*int_base 9*/);
    primitive(65726L /*"relpenalty"*/, 74 /*assign_int*/, 8938750L /*int_base 10*/);
    primitive(65727L /*"predisplaypenalty"*/, 74 /*assign_int*/, 8938751L /*int_base 11*/);
    primitive(65728L /*"postdisplaypenalty"*/, 74 /*assign_int*/, 8938752L /*int_base 12*/);
    primitive(65729L /*"interlinepenalty"*/, 74 /*assign_int*/, 8938753L /*int_base 13*/);
    primitive(65730L /*"doublehyphendemerits"*/, 74 /*assign_int*/, 8938754L /*int_base 14*/);
    primitive(65731L /*"finalhyphendemerits"*/, 74 /*assign_int*/, 8938755L /*int_base 15*/);
    primitive(65732L /*"adjdemerits"*/, 74 /*assign_int*/, 8938756L /*int_base 16*/);
    primitive(65733L /*"mag"*/, 74 /*assign_int*/, 8938757L /*int_base 17*/);
    primitive(65734L /*"delimiterfactor"*/, 74 /*assign_int*/, 8938758L /*int_base 18*/);
    primitive(65735L /*"looseness"*/, 74 /*assign_int*/, 8938759L /*int_base 19*/);
    primitive(65736L /*"time"*/, 74 /*assign_int*/, 8938760L /*int_base 20*/);
    primitive(65737L /*"day"*/, 74 /*assign_int*/, 8938761L /*int_base 21*/);
    primitive(65738L /*"month"*/, 74 /*assign_int*/, 8938762L /*int_base 22*/);
    primitive(65739L /*"year"*/, 74 /*assign_int*/, 8938763L /*int_base 23*/);
    primitive(65740L /*"showboxbreadth"*/, 74 /*assign_int*/, 8938764L /*int_base 24*/);
    primitive(65741L /*"showboxdepth"*/, 74 /*assign_int*/, 8938765L /*int_base 25*/);
    primitive(65742L /*"hbadness"*/, 74 /*assign_int*/, 8938766L /*int_base 26*/);
    primitive(65743L /*"vbadness"*/, 74 /*assign_int*/, 8938767L /*int_base 27*/);
    primitive(65744L /*"pausing"*/, 74 /*assign_int*/, 8938768L /*int_base 28*/);
    primitive(65745L /*"tracingonline"*/, 74 /*assign_int*/, 8938769L /*int_base 29*/);
    primitive(65746L /*"tracingmacros"*/, 74 /*assign_int*/, 8938770L /*int_base 30*/);
    primitive(65747L /*"tracingstats"*/, 74 /*assign_int*/, 8938771L /*int_base 31*/);
    primitive(65748L /*"tracingparagraphs"*/, 74 /*assign_int*/, 8938772L /*int_base 32*/);
    primitive(65749L /*"tracingpages"*/, 74 /*assign_int*/, 8938773L /*int_base 33*/);
    primitive(65750L /*"tracingoutput"*/, 74 /*assign_int*/, 8938774L /*int_base 34*/);
    primitive(65751L /*"tracinglostchars"*/, 74 /*assign_int*/, 8938775L /*int_base 35*/);
    primitive(65752L /*"tracingcommands"*/, 74 /*assign_int*/, 8938776L /*int_base 36*/);
    primitive(65753L /*"tracingrestores"*/, 74 /*assign_int*/, 8938777L /*int_base 37*/);
    primitive(65754L /*"uchyph"*/, 74 /*assign_int*/, 8938778L /*int_base 38*/);
    primitive(65755L /*"outputpenalty"*/, 74 /*assign_int*/, 8938779L /*int_base 39*/);
    primitive(65756L /*"maxdeadcycles"*/, 74 /*assign_int*/, 8938780L /*int_base 40*/);
    primitive(65757L /*"hangafter"*/, 74 /*assign_int*/, 8938781L /*int_base 41*/);
    primitive(65758L /*"floatingpenalty"*/, 74 /*assign_int*/, 8938782L /*int_base 42*/);
    primitive(65759L /*"globaldefs"*/, 74 /*assign_int*/, 8938783L /*int_base 43*/);
    primitive(65760L /*"fam"*/, 74 /*assign_int*/, 8938784L /*int_base 44*/);
    primitive(65761L /*"escapechar"*/, 74 /*assign_int*/, 8938785L /*int_base 45*/);
    primitive(65762L /*"defaulthyphenchar"*/, 74 /*assign_int*/, 8938786L /*int_base 46*/);
    primitive(65763L /*"defaultskewchar"*/, 74 /*assign_int*/, 8938787L /*int_base 47*/);
    primitive(65764L /*"endlinechar"*/, 74 /*assign_int*/, 8938788L /*int_base 48*/);
    primitive(65765L /*"newlinechar"*/, 74 /*assign_int*/, 8938789L /*int_base 49*/);
    primitive(65766L /*"language"*/, 74 /*assign_int*/, 8938790L /*int_base 50*/);
    primitive(65767L /*"lefthyphenmin"*/, 74 /*assign_int*/, 8938791L /*int_base 51*/);
    primitive(65768L /*"righthyphenmin"*/, 74 /*assign_int*/, 8938792L /*int_base 52*/);
    primitive(65769L /*"holdinginserts"*/, 74 /*assign_int*/, 8938793L /*int_base 53*/);
    primitive(65770L /*"errorcontextlines"*/, 74 /*assign_int*/, 8938794L /*int_base 54*/);

    if (mltex_p) {
        mltex_enabled_p = true;
        primitive(65772L /*"charsubdefmax"*/, 74 /*assign_int*/, 8938796L /*int_base 56*/);
        primitive(65773L /*"tracingcharsubdef"*/, 74 /*assign_int*/, 8938797L /*int_base 57*/);
    }

    primitive(65774L /*"XeTeXlinebreakpenalty"*/, 74 /*assign_int*/, 8938809L /*int_base 69*/);
    primitive(65775L /*"XeTeXprotrudechars"*/, 74 /*assign_int*/, 8938810L /*int_base 70*/);
    primitive(65779L /*"parindent"*/, 75 /*assign_dimen*/, 10053192L /*dimen_base 0*/);
    primitive(65780L /*"mathsurround"*/, 75 /*assign_dimen*/, 10053193L /*dimen_base 1*/);
    primitive(65781L /*"lineskiplimit"*/, 75 /*assign_dimen*/, 10053194L /*dimen_base 2*/);
    primitive(65782L /*"hsize"*/, 75 /*assign_dimen*/, 10053195L /*dimen_base 3*/);
    primitive(65783L /*"vsize"*/, 75 /*assign_dimen*/, 10053196L /*dimen_base 4*/);
    primitive(65784L /*"maxdepth"*/, 75 /*assign_dimen*/, 10053197L /*dimen_base 5*/);
    primitive(65785L /*"splitmaxdepth"*/, 75 /*assign_dimen*/, 10053198L /*dimen_base 6*/);
    primitive(65786L /*"boxmaxdepth"*/, 75 /*assign_dimen*/, 10053199L /*dimen_base 7*/);
    primitive(65787L /*"hfuzz"*/, 75 /*assign_dimen*/, 10053200L /*dimen_base 8*/);
    primitive(65788L /*"vfuzz"*/, 75 /*assign_dimen*/, 10053201L /*dimen_base 9*/);
    primitive(65789L /*"delimitershortfall"*/, 75 /*assign_dimen*/, 10053202L /*dimen_base 10*/);
    primitive(65790L /*"nulldelimiterspace"*/, 75 /*assign_dimen*/, 10053203L /*dimen_base 11*/);
    primitive(65791L /*"scriptspace"*/, 75 /*assign_dimen*/, 10053204L /*dimen_base 12*/);
    primitive(65792L /*"predisplaysize"*/, 75 /*assign_dimen*/, 10053205L /*dimen_base 13*/);
    primitive(65793L /*"displaywidth"*/, 75 /*assign_dimen*/, 10053206L /*dimen_base 14*/);
    primitive(65794L /*"displayindent"*/, 75 /*assign_dimen*/, 10053207L /*dimen_base 15*/);
    primitive(65795L /*"overfullrule"*/, 75 /*assign_dimen*/, 10053208L /*dimen_base 16*/);
    primitive(65796L /*"hangindent"*/, 75 /*assign_dimen*/, 10053209L /*dimen_base 17*/);
    primitive(65797L /*"hoffset"*/, 75 /*assign_dimen*/, 10053210L /*dimen_base 18*/);
    primitive(65798L /*"voffset"*/, 75 /*assign_dimen*/, 10053211L /*dimen_base 19*/);
    primitive(65799L /*"emergencystretch"*/, 75 /*assign_dimen*/, 10053212L /*dimen_base 20*/);
    primitive(65800L /*"pdfpagewidth"*/, 75 /*assign_dimen*/, 10053213L /*dimen_base 21*/);
    primitive(65801L /*"pdfpageheight"*/, 75 /*assign_dimen*/, 10053214L /*dimen_base 22*/);
    primitive(32 /*" "*/, 64 /*ex_space*/, 0);
    primitive(47 /*"/"*/, 44 /*ital_corr*/, 0);
    primitive(65813L /*"accent"*/, 45 /*accent*/, 0);
    primitive(65814L /*"advance"*/, 92 /*advance*/, 0);
    primitive(65815L /*"afterassignment"*/, 40 /*after_assignment*/, 0);
    primitive(65816L /*"aftergroup"*/, 41 /*after_group*/, 0);
    primitive(65817L /*"begingroup"*/, 61 /*begin_group*/, 0);
    primitive(65818L /*"char"*/, 16 /*char_num*/, 0);
    primitive(65809L /*"csname"*/, 109 /*cs_name*/, 0);
    primitive(65819L /*"delimiter"*/, 15 /*delim_num*/, 0);
    primitive(65820L /*"XeTeXdelimiter"*/, 15 /*delim_num*/, 1);
    primitive(65821L /*"Udelimiter"*/, 15 /*delim_num*/, 1);
    primitive(65822L /*"divide"*/, 94 /*divide*/, 0);
    primitive(65810L /*"endcsname"*/, 67 /*end_cs_name*/, 0);
    primitive(65823L /*"endgroup"*/, 62 /*end_group*/, 0);
    hash[2243228L /*frozen_end_group*/].v.RH = 65823L /*"endgroup"*/;
    eqtb[2243228L /*frozen_end_group*/] = eqtb[cur_val];
    primitive(65824L /*"expandafter"*/, 104 /*expand_after*/, 0);
    primitive(65825L /*"font"*/, 90 /*def_font*/, 0);
    primitive(65826L /*"fontdimen"*/, 78 /*assign_font_dimen*/, 0);
    primitive(65827L /*"halign"*/, 32 /*halign*/, 0);
    primitive(65828L /*"hrule"*/, 36 /*hrule*/, 0);
    primitive(65829L /*"ignorespaces"*/, 39 /*ignore_spaces*/, 0);
    primitive(65614L /*"insert"*/, 37 /*insert*/, 0);
    primitive(65637L /*"mark"*/, 18 /*mark*/, 0);
    primitive(65830L /*"mathaccent"*/, 46 /*math_accent*/, 0);
    primitive(65831L /*"XeTeXmathaccent"*/, 46 /*math_accent*/, 1);
    primitive(65832L /*"Umathaccent"*/, 46 /*math_accent*/, 1);
    primitive(65833L /*"mathchar"*/, 17 /*math_char_num*/, 0);
    primitive(65834L /*"XeTeXmathcharnum"*/, 17 /*math_char_num*/, 1);
    primitive(65835L /*"Umathcharnum"*/, 17 /*math_char_num*/, 1);
    primitive(65836L /*"XeTeXmathchar"*/, 17 /*math_char_num*/, 2);
    primitive(65837L /*"Umathchar"*/, 17 /*math_char_num*/, 2);
    primitive(65838L /*"mathchoice"*/, 54 /*math_choice*/, 0);
    primitive(65839L /*"multiply"*/, 93 /*multiply*/, 0);
    primitive(65840L /*"noalign"*/, 34 /*no_align*/, 0);
    primitive(65841L /*"noboundary"*/, 65 /*no_boundary*/, 0);
    primitive(65842L /*"noexpand"*/, 105 /*no_expand*/, 0);
    primitive(65806L /*"primitive"*/, 105 /*no_expand*/, 1);
    primitive(65619L /*"nonscript"*/, 55 /*non_script*/, 0);
    primitive(65843L /*"omit"*/, 63 /*omit*/, 0);
    primitive(65844L /*"parshape"*/, 85 /*set_shape*/, 2252771L /*par_shape_loc*/);
    primitive(65845L /*"penalty"*/, 42 /*break_penalty*/, 0);
    primitive(65846L /*"prevgraf"*/, 81 /*set_prev_graf*/, 0);
    primitive(65847L /*"radical"*/, 66 /*radical*/, 0);
    primitive(65848L /*"XeTeXradical"*/, 66 /*radical*/, 1);
    primitive(65849L /*"Uradical"*/, 66 /*radical*/, 1);
    primitive(65850L /*"read"*/, 98 /*read_to_cs*/, 0);
    primitive(65851L /*"relax"*/, 0 /*relax*/, 1114112L /*too_big_usv*/);
    hash[2243233L /*frozen_relax*/].v.RH = 65851L /*"relax"*/;
    eqtb[2243233L /*frozen_relax*/] = eqtb[cur_val];
    primitive(65852L /*"setbox"*/, 100 /*set_box*/, 0);
    primitive(65853L /*"the"*/, 111 /*the*/, 0);
    primitive(65703L /*"toks"*/, 72 /*toks_register*/, mem_bot);
    primitive(65638L /*"vadjust"*/, 38 /*vadjust*/, 0);
    primitive(65854L /*"valign"*/, 33 /*valign*/, 0);
    primitive(65855L /*"vcenter"*/, 56 /*vcenter*/, 0);
    primitive(65856L /*"vrule"*/, 35 /*vrule*/, 0);
    primitive(65917L /*"par"*/, 13 /*par_end*/, 1114112L /*too_big_usv*/);
    par_loc = cur_val;
    par_token = 33554431L /*cs_token_flag*/ + par_loc;
    primitive(65952L /*"input"*/, 106 /*input*/, 0);
    primitive(65953L /*"endinput"*/, 106 /*input*/, 1);
    primitive(65954L /*"topmark"*/, 112 /*top_bot_mark*/, 0 /*top_mark_code*/);
    primitive(65955L /*"firstmark"*/, 112 /*top_bot_mark*/, 1 /*first_mark_code*/);
    primitive(65956L /*"botmark"*/, 112 /*top_bot_mark*/, 2 /*bot_mark_code*/);
    primitive(65957L /*"splitfirstmark"*/, 112 /*top_bot_mark*/, 3 /*split_first_mark_code*/);
    primitive(65958L /*"splitbotmark"*/, 112 /*top_bot_mark*/, 4 /*split_bot_mark_code*/);
    primitive(65777L /*"count"*/, 91 /*register*/, mem_bot + 0);
    primitive(65803L /*"dimen"*/, 91 /*register*/, mem_bot + 1);
    primitive(65691L /*"skip"*/, 91 /*register*/, mem_bot + 2);
    primitive(65692L /*"muskip"*/, 91 /*register*/, mem_bot + 3);
    primitive(66002L /*"spacefactor"*/, 80 /*set_aux*/, 104 /*hmode*/);
    primitive(66003L /*"prevdepth"*/, 80 /*set_aux*/, 1 /*vmode*/);
    primitive(66004L /*"deadcycles"*/, 83 /*set_page_int*/, 0);
    primitive(66005L /*"insertpenalties"*/, 83 /*set_page_int*/, 1);
    primitive(66006L /*"wd"*/, 84 /*set_box_dimen*/, 1 /*width_offset*/);
    primitive(66007L /*"ht"*/, 84 /*set_box_dimen*/, 3 /*height_offset*/);
    primitive(66008L /*"dp"*/, 84 /*set_box_dimen*/, 2 /*depth_offset*/);
    primitive(66009L /*"lastpenalty"*/, 71 /*last_item*/, 0 /*int_val*/);
    primitive(66010L /*"lastkern"*/, 71 /*last_item*/, 1 /*dimen_val*/);
    primitive(66011L /*"lastskip"*/, 71 /*last_item*/, 2 /*glue_val*/);
    primitive(66012L /*"inputlineno"*/, 71 /*last_item*/, 4 /*input_line_no_code*/);
    primitive(66013L /*"badness"*/, 71 /*last_item*/, 5 /*badness_code*/);
    primitive(66082L /*"number"*/, 110 /*convert*/, 0 /*number_code*/);
    primitive(66083L /*"romannumeral"*/, 110 /*convert*/, 1 /*roman_numeral_code*/);
    primitive(66084L /*"string"*/, 110 /*convert*/, 2 /*string_code*/);
    primitive(66085L /*"meaning"*/, 110 /*convert*/, 3 /*meaning_code*/);
    primitive(66086L /*"fontname"*/, 110 /*convert*/, 4 /*font_name_code*/);
    primitive(66087L /*"jobname"*/, 110 /*convert*/, 15 /*job_name_code*/);
    primitive(66088L /*"leftmarginkern"*/, 110 /*convert*/, 11 /*left_margin_kern_code*/);
    primitive(66089L /*"rightmarginkern"*/, 110 /*convert*/, 12 /*right_margin_kern_code*/);
    primitive(66090L /*"Uchar"*/, 110 /*convert*/, 13 /*XeTeX_Uchar_code*/);
    primitive(66091L /*"Ucharcat"*/, 110 /*convert*/, 14 /*XeTeX_Ucharcat_code*/);
    primitive(66112L /*"if"*/, 107 /*if_test*/, 0 /*if_char_code*/);
    primitive(66113L /*"ifcat"*/, 107 /*if_test*/, 1 /*if_cat_code*/);
    primitive(66114L /*"ifnum"*/, 107 /*if_test*/, 2 /*if_int_code*/);
    primitive(66115L /*"ifdim"*/, 107 /*if_test*/, 3 /*if_dim_code*/);
    primitive(66116L /*"ifodd"*/, 107 /*if_test*/, 4 /*if_odd_code*/);
    primitive(66117L /*"ifvmode"*/, 107 /*if_test*/, 5 /*if_vmode_code*/);
    primitive(66118L /*"ifhmode"*/, 107 /*if_test*/, 6 /*if_hmode_code*/);
    primitive(66119L /*"ifmmode"*/, 107 /*if_test*/, 7 /*if_mmode_code*/);
    primitive(66120L /*"ifinner"*/, 107 /*if_test*/, 8 /*if_inner_code*/);
    primitive(66121L /*"ifvoid"*/, 107 /*if_test*/, 9 /*if_void_code*/);
    primitive(66122L /*"ifhbox"*/, 107 /*if_test*/, 10 /*if_hbox_code*/);
    primitive(66123L /*"ifvbox"*/, 107 /*if_test*/, 11 /*if_vbox_code*/);
    primitive(66124L /*"ifx"*/, 107 /*if_test*/, 12 /*ifx_code*/);
    primitive(66125L /*"ifeof"*/, 107 /*if_test*/, 13 /*if_eof_code*/);
    primitive(66126L /*"iftrue"*/, 107 /*if_test*/, 14 /*if_true_code*/);
    primitive(66127L /*"iffalse"*/, 107 /*if_test*/, 15 /*if_false_code*/);
    primitive(66128L /*"ifcase"*/, 107 /*if_test*/, 16 /*if_case_code*/);
    primitive(66129L /*"ifprimitive"*/, 107 /*if_test*/, 21 /*if_primitive_code*/);
    primitive(66131L /*"fi"*/, 108 /*fi_or_else*/, 2 /*fi_code*/);
    hash[2243230L /*frozen_fi*/].v.RH = 66131L /*"fi"*/;
    eqtb[2243230L /*frozen_fi*/] = eqtb[cur_val];
    primitive(66132L /*"or"*/, 108 /*fi_or_else*/, 4 /*or_code*/);
    primitive(66133L /*"else"*/, 108 /*fi_or_else*/, 3 /*else_code*/);
    primitive(66159L /*"nullfont"*/, 89 /*set_font*/, 0 /*font_base*/);
    hash[2243238L /*frozen_null_font*/].v.RH = 66159L /*"nullfont"*/;
    eqtb[2243238L /*frozen_null_font*/] = eqtb[cur_val];
    primitive(66288L /*"span"*/, 4 /*tab_mark*/, 65537L /*span_code*/);
    primitive(66289L /*"cr"*/, 5 /*car_ret*/, 65538L /*cr_code*/);
    hash[2243227L /*frozen_cr*/].v.RH = 66289L /*"cr"*/;
    eqtb[2243227L /*frozen_cr*/] = eqtb[cur_val];
    primitive(66290L /*"crcr"*/, 5 /*car_ret*/, 65539L /*cr_cr_code*/);
    hash[2243231L /*frozen_end_template*/].v.RH = 66291L /*"endtemplate"*/;
    hash[2243232L /*frozen_endv*/].v.RH = 66291L /*"endtemplate"*/;
    eqtb[2243232L /*frozen_endv*/].hh.u.B0 = 9 /*endv*/;
    eqtb[2243232L /*frozen_endv*/].hh.v.RH = mem_top - 11;
    eqtb[2243232L /*frozen_endv*/].hh.u.B1 = 1 /*level_one*/;
    eqtb[2243231L /*frozen_end_template*/] = eqtb[2243232L /*frozen_endv*/];
    eqtb[2243231L /*frozen_end_template*/].hh.u.B0 = 117 /*end_template*/;
    primitive(66368L /*"pagegoal"*/, 82 /*set_page_dimen*/, 0);
    primitive(66369L /*"pagetotal"*/, 82 /*set_page_dimen*/, 1);
    primitive(66370L /*"pagestretch"*/, 82 /*set_page_dimen*/, 2);
    primitive(66371L /*"pagefilstretch"*/, 82 /*set_page_dimen*/, 3);
    primitive(66372L /*"pagefillstretch"*/, 82 /*set_page_dimen*/, 4);
    primitive(66373L /*"pagefilllstretch"*/, 82 /*set_page_dimen*/, 5);
    primitive(66374L /*"pageshrink"*/, 82 /*set_page_dimen*/, 6);
    primitive(66375L /*"pagedepth"*/, 82 /*set_page_dimen*/, 7);
    primitive(65627L /*"end"*/, 14 /*stop*/, 0);
    primitive(66422L /*"dump"*/, 14 /*stop*/, 1);
    primitive(66423L /*"hskip"*/, 26 /*hskip*/, 4 /*skip_code*/);
    primitive(66424L /*"hfil"*/, 26 /*hskip*/, 0 /*fil_code*/);
    primitive(66425L /*"hfill"*/, 26 /*hskip*/, 1 /*fill_code*/);
    primitive(66426L /*"hss"*/, 26 /*hskip*/, 2 /*ss_code*/);
    primitive(66427L /*"hfilneg"*/, 26 /*hskip*/, 3 /*fil_neg_code*/);
    primitive(66428L /*"vskip"*/, 27 /*vskip*/, 4 /*skip_code*/);
    primitive(66429L /*"vfil"*/, 27 /*vskip*/, 0 /*fil_code*/);
    primitive(66430L /*"vfill"*/, 27 /*vskip*/, 1 /*fill_code*/);
    primitive(66431L /*"vss"*/, 27 /*vskip*/, 2 /*ss_code*/);
    primitive(66432L /*"vfilneg"*/, 27 /*vskip*/, 3 /*fil_neg_code*/);
    primitive(65620L /*"mskip"*/, 28 /*mskip*/, 5 /*mskip_code*/);
    primitive(65599L /*"kern"*/, 29 /*kern*/, 1 /*explicit*/);
    primitive(65626L /*"mkern"*/, 30 /*mkern*/, 99 /*mu_glue*/);
    primitive(66450L /*"moveleft"*/, 21 /*hmove*/, 1);
    primitive(66451L /*"moveright"*/, 21 /*hmove*/, 0);
    primitive(66452L /*"raise"*/, 22 /*vmove*/, 1);
    primitive(66453L /*"lower"*/, 22 /*vmove*/, 0);
    primitive(65705L /*"box"*/, 20 /*make_box*/, 0 /*box_code*/);
    primitive(66454L /*"copy"*/, 20 /*make_box*/, 1 /*copy_code*/);
    primitive(66455L /*"lastbox"*/, 20 /*make_box*/, 2 /*last_box_code*/);
    primitive(66363L /*"vsplit"*/, 20 /*make_box*/, 3 /*vsplit_code*/);
    primitive(66456L /*"vtop"*/, 20 /*make_box*/, 4 /*vtop_code*/);
    primitive(66365L /*"vbox"*/, 20 /*make_box*/, 5 /*vtop_code 1*/);
    primitive(66457L /*"hbox"*/, 20 /*make_box*/, 108 /*vtop_code 104*/);
    primitive(66458L /*"shipout"*/, 31 /*leader_ship*/, 99 /*a_leaders -1*/);
    primitive(66459L /*"leaders"*/, 31 /*leader_ship*/, 100 /*a_leaders*/);
    primitive(66460L /*"cleaders"*/, 31 /*leader_ship*/, 101 /*c_leaders*/);
    primitive(66461L /*"xleaders"*/, 31 /*leader_ship*/, 102 /*x_leaders*/);
    primitive(66477L /*"indent"*/, 43 /*start_par*/, 1);
    primitive(66478L /*"noindent"*/, 43 /*start_par*/, 0);
    primitive(66488L /*"unpenalty"*/, 25 /*remove_item*/, 12 /*penalty_node*/);
    primitive(66489L /*"unkern"*/, 25 /*remove_item*/, 11 /*kern_node*/);
    primitive(66490L /*"unskip"*/, 25 /*remove_item*/, 10 /*glue_node*/);
    primitive(66491L /*"unhbox"*/, 23 /*un_hbox*/, 0 /*box_code*/);
    primitive(66492L /*"unhcopy"*/, 23 /*un_hbox*/, 1 /*copy_code*/);
    primitive(66493L /*"unvbox"*/, 24 /*un_vbox*/, 0 /*box_code*/);
    primitive(66494L /*"unvcopy"*/, 24 /*un_vbox*/, 1 /*copy_code*/);
    primitive(45 /*"-"*/, 47 /*discretionary*/, 1);
    primitive(65635L /*"discretionary"*/, 47 /*discretionary*/, 0);
    primitive(66525L /*"eqno"*/, 48 /*eq_no*/, 0);
    primitive(66526L /*"leqno"*/, 48 /*eq_no*/, 1);
    primitive(66238L /*"mathord"*/, 50 /*math_comp*/, 16 /*ord_noad*/);
    primitive(66239L /*"mathop"*/, 50 /*math_comp*/, 17 /*op_noad*/);
    primitive(66240L /*"mathbin"*/, 50 /*math_comp*/, 18 /*bin_noad*/);
    primitive(66241L /*"mathrel"*/, 50 /*math_comp*/, 19 /*rel_noad*/);
    primitive(66242L /*"mathopen"*/, 50 /*math_comp*/, 20 /*open_noad*/);
    primitive(66243L /*"mathclose"*/, 50 /*math_comp*/, 21 /*close_noad*/);
    primitive(66244L /*"mathpunct"*/, 50 /*math_comp*/, 22 /*punct_noad*/);
    primitive(66245L /*"mathinner"*/, 50 /*math_comp*/, 23 /*inner_noad*/);
    primitive(66247L /*"underline"*/, 50 /*math_comp*/, 26 /*under_noad*/);
    primitive(66246L /*"overline"*/, 50 /*math_comp*/, 27 /*over_noad*/);
    primitive(66527L /*"displaylimits"*/, 51 /*limit_switch*/, 0 /*normal*/);
    primitive(66251L /*"limits"*/, 51 /*limit_switch*/, 1 /*limits*/);
    primitive(66252L /*"nolimits"*/, 51 /*limit_switch*/, 2 /*no_limits*/);
    primitive(66233L /*"displaystyle"*/, 53 /*math_style*/, 0 /*display_style*/);
    primitive(66234L /*"textstyle"*/, 53 /*math_style*/, 2 /*text_style*/);
    primitive(66235L /*"scriptstyle"*/, 53 /*math_style*/, 4 /*script_style*/);
    primitive(66236L /*"scriptscriptstyle"*/, 53 /*math_style*/, 6 /*script_script_style*/);
    primitive(66547L /*"above"*/, 52 /*above*/, 0 /*above_code*/);
    primitive(66548L /*"over"*/, 52 /*above*/, 1 /*over_code*/);
    primitive(66549L /*"atop"*/, 52 /*above*/, 2 /*atop_code*/);
    primitive(66550L /*"abovewithdelims"*/, 52 /*above*/, 3 /*delimited_code 0*/);
    primitive(66551L /*"overwithdelims"*/, 52 /*above*/, 4 /*delimited_code 1*/);
    primitive(66552L /*"atopwithdelims"*/, 52 /*above*/, 5 /*delimited_code 2*/);
    primitive(66248L /*"left"*/, 49 /*left_right*/, 30 /*left_noad*/);
    primitive(66249L /*"right"*/, 49 /*left_right*/, 31 /*right_noad*/);
    hash[2243229L /*frozen_right*/].v.RH = 66249L /*"right"*/;
    eqtb[2243229L /*frozen_right*/] = eqtb[cur_val];
    primitive(66572L /*"long"*/, 95 /*prefix*/, 1);
    primitive(66573L /*"outer"*/, 95 /*prefix*/, 2);
    primitive(66574L /*"global"*/, 95 /*prefix*/, 4);
    primitive(66575L /*"def"*/, 99 /*def*/, 0);
    primitive(66576L /*"gdef"*/, 99 /*def*/, 1);
    primitive(66577L /*"edef"*/, 99 /*def*/, 2);
    primitive(66578L /*"xdef"*/, 99 /*def*/, 3);
    primitive(66595L /*"let"*/, 96 /*let*/, 0 /*normal*/);
    primitive(66596L /*"futurelet"*/, 96 /*let*/, 1 /*normal 1*/);
    primitive(66597L /*"chardef"*/, 97 /*shorthand_def*/, 0 /*char_def_code*/);
    primitive(66598L /*"mathchardef"*/, 97 /*shorthand_def*/, 1 /*math_char_def_code*/);
    primitive(66599L /*"XeTeXmathcharnumdef"*/, 97 /*shorthand_def*/, 8 /*XeTeX_math_char_num_def_code*/);
    primitive(66600L /*"Umathcharnumdef"*/, 97 /*shorthand_def*/, 8 /*XeTeX_math_char_num_def_code*/);
    primitive(66601L /*"XeTeXmathchardef"*/, 97 /*shorthand_def*/, 9 /*XeTeX_math_char_def_code*/);
    primitive(66602L /*"Umathchardef"*/, 97 /*shorthand_def*/, 9 /*XeTeX_math_char_def_code*/);
    primitive(66603L /*"countdef"*/, 97 /*shorthand_def*/, 2 /*count_def_code*/);
    primitive(66604L /*"dimendef"*/, 97 /*shorthand_def*/, 3 /*dimen_def_code*/);
    primitive(66605L /*"skipdef"*/, 97 /*shorthand_def*/, 4 /*skip_def_code*/);
    primitive(66606L /*"muskipdef"*/, 97 /*shorthand_def*/, 5 /*mu_skip_def_code*/);
    primitive(66607L /*"toksdef"*/, 97 /*shorthand_def*/, 6 /*toks_def_code*/);

    if (mltex_p)
        primitive(66608L /*"charsubdef"*/, 97 /*shorthand_def*/, 7 /*char_sub_def_code*/);

    primitive(65711L /*"catcode"*/, 86 /*def_code*/, 2254068L /*cat_code_base*/);
    primitive(65715L /*"mathcode"*/, 86 /*def_code*/, 6710516L /*math_code_base*/);
    primitive(66613L /*"XeTeXmathcodenum"*/, 87 /*XeTeX_def_code*/, 6710516L /*math_code_base*/);
    primitive(66614L /*"Umathcodenum"*/, 87 /*XeTeX_def_code*/, 6710516L /*math_code_base*/);
    primitive(66615L /*"XeTeXmathcode"*/, 87 /*XeTeX_def_code*/, 6710517L /*math_code_base 1*/);
    primitive(66616L /*"Umathcode"*/, 87 /*XeTeX_def_code*/, 6710517L /*math_code_base 1*/);
    primitive(65712L /*"lccode"*/, 86 /*def_code*/, 3368180L /*lc_code_base*/);
    primitive(65713L /*"uccode"*/, 86 /*def_code*/, 4482292L /*uc_code_base*/);
    primitive(65714L /*"sfcode"*/, 86 /*def_code*/, 5596404L /*sf_code_base*/);
    primitive(66617L /*"XeTeXcharclass"*/, 87 /*XeTeX_def_code*/, 5596404L /*sf_code_base*/);
    primitive(65778L /*"delcode"*/, 86 /*def_code*/, 8939080L /*del_code_base*/);
    primitive(66618L /*"XeTeXdelcodenum"*/, 87 /*XeTeX_def_code*/, 8939080L /*del_code_base*/);
    primitive(66619L /*"Udelcodenum"*/, 87 /*XeTeX_def_code*/, 8939080L /*del_code_base*/);
    primitive(66620L /*"XeTeXdelcode"*/, 87 /*XeTeX_def_code*/, 8939081L /*del_code_base 1*/);
    primitive(66621L /*"Udelcode"*/, 87 /*XeTeX_def_code*/, 8939081L /*del_code_base 1*/);
    primitive(65708L /*"textfont"*/, 88 /*def_family*/, 2253300L /*math_font_base*/);
    primitive(65709L /*"scriptfont"*/, 88 /*def_family*/, 2253556L /*math_font_base 256*/);
    primitive(65710L /*"scriptscriptfont"*/, 88 /*def_family*/, 2253812L /*math_font_base 512*/);
    primitive(66339L /*"hyphenation"*/, 101 /*hyph_data*/, 0);
    primitive(66351L /*"patterns"*/, 101 /*hyph_data*/, 1);
    primitive(66636L /*"hyphenchar"*/, 79 /*assign_font_int*/, 0);
    primitive(66637L /*"skewchar"*/, 79 /*assign_font_int*/, 1);
    primitive(66638L /*"lpcode"*/, 79 /*assign_font_int*/, 2);
    primitive(66639L /*"rpcode"*/, 79 /*assign_font_int*/, 3);
    primitive(65554L /*"batchmode"*/, 102 /*set_interaction*/, 0 /*batch_mode*/);
    primitive(65555L /*"nonstopmode"*/, 102 /*set_interaction*/, 1 /*nonstop_mode*/);
    primitive(65556L /*"scrollmode"*/, 102 /*set_interaction*/, 2 /*scroll_mode*/);
    primitive(66648L /*"errorstopmode"*/, 102 /*set_interaction*/, 3 /*error_stop_mode*/);
    primitive(66649L /*"openin"*/, 60 /*in_stream*/, 1);
    primitive(66650L /*"closein"*/, 60 /*in_stream*/, 0);
    primitive(66651L /*"message"*/, 58 /*message*/, 0);
    primitive(66652L /*"errmessage"*/, 58 /*message*/, 1);
    primitive(66658L /*"lowercase"*/, 57 /*case_shift*/, 3368180L /*lc_code_base*/);
    primitive(66659L /*"uppercase"*/, 57 /*case_shift*/, 4482292L /*uc_code_base*/);
    primitive(66660L /*"show"*/, 19 /*xray*/, 0 /*show_code*/);
    primitive(66661L /*"showbox"*/, 19 /*xray*/, 1 /*show_box_code*/);
    primitive(66662L /*"showthe"*/, 19 /*xray*/, 2 /*show_the_code*/);
    primitive(66663L /*"showlists"*/, 19 /*xray*/, 3 /*show_lists*/);
    primitive(66711L /*"openout"*/, 59 /*extension*/, 0 /*open_node*/);
    primitive(65914L /*"write"*/, 59 /*extension*/, 1 /*write_node*/);
    write_loc = cur_val;
    primitive(66712L /*"closeout"*/, 59 /*extension*/, 2 /*close_node*/);
    primitive(66713L /*"special"*/, 59 /*extension*/, 3 /*special_node*/);
    hash[2243236L /*frozen_special*/].v.RH = 66713L /*"special"*/;
    eqtb[2243236L /*frozen_special*/] = eqtb[cur_val];
    primitive(66714L /*"immediate"*/, 59 /*extension*/, 4 /*immediate_code*/);
    primitive(66715L /*"setlanguage"*/, 59 /*extension*/, 5 /*set_language_code*/);
    primitive(66945L /*"synctex"*/, 74 /*assign_int*/, 8938823L /*int_base 83*/);

    no_new_control_sequence = true;
}


static boolean
get_strings_started(void)
{
    pool_ptr = 0;
    str_ptr = 0;
    str_start[0] = 0;
    str_ptr = 65536L /*too_big_char*/;
    str_start[(str_ptr) - 65536L] = pool_ptr;

    if (load_pool_strings(pool_size - string_vacancies) == 0)
	_tt_abort ("must increase pool_size");

    return true;
}/*:1001*/


/* Initialization bits that were in the C driver code */


void
tt_misc_initialize(char *dump_name)
{
    /* Miscellaneous initializations that were mostly originally done in the
     * main() driver routines. */

    /* Get our stdout handle */

    rust_stdout = ttstub_output_open_stdout ();

    /* TEX_format_default must get a leading space character for Pascal
     * style string magic. */

    size_t len = strlen (dump_name);
    TEX_format_default = xmalloc (len + 2);
    TEX_format_default[0] = ' ';
    strcpy (TEX_format_default + 1, dump_name);
    format_default_length = len + 2;

    /* Not sure why these get custom initializations. */

    interaction_option = 4;
    synctex_options = INT_MAX;

    if (file_line_error_style_p < 0)
	file_line_error_style_p = 0;

    /* Make this something invariant so that we can use XDV files to test
     * reproducibility of the engine output. */

    output_comment = "tectonic";
}

/*:1371*//*1373: */

/* setjmp handing of fatal errors. I tried to compartmentalize this code in
 * errors.c but it seems that wrapping setjmp() in a little function does not
 * work. */

#define BUF_SIZE 1024

static jmp_buf jump_buffer;
static char error_buf[BUF_SIZE] = "";

NORETURN PRINTF_FUNC(1,2) int
_tt_abort (const_string format, ...)
{
    va_list ap;

    va_start (ap, format);
    vsnprintf (error_buf, BUF_SIZE, format, ap);
    va_end (ap);
    longjmp (jump_buffer, 1);
}

const const_string
tt_get_error_message (void)
{
    return error_buf;
}


tt_history_t
tt_run_engine(char *input_file_name)
{
    /* FKA main_body() */
    memory_word *eqtb = zeqtb;

    /* Before anything else ... setjmp handling of super-fatal errors */

    if (setjmp (jump_buffer)) {
	history = HISTORY_FATAL_ERROR;
	return history;
    }

    /* These various parameters were configurable in web2c TeX. We don't
     * bother to allow that. */

    mem_bot = 0;
    main_memory = 5000000L;
    extra_mem_top = 0;
    extra_mem_bot = 0;
    pool_size = 6250000L;
    string_vacancies = 90000L;
    pool_free = 47500L;
    max_strings = 565536L;
    strings_free = 100;
    font_mem_size = 8000000L;
    font_max = 9000;
    trie_size = 1000000L;
    hyph_size = 8191;
    buf_size = 200000L;
    nest_size = 500;
    max_in_open = 15;
    param_size = 10000;
    save_size = 80000L;
    stack_size = 5000;
    dvi_buf_size = 16384;
    error_line = 79;
    half_error_line = 50;
    max_print_line = 79;
    hash_extra = 600000L;
    expand_depth = 10000;

    if (in_initex_mode) {
        extra_mem_top = 0;
        extra_mem_bot = 0;
    }

    mem_top = mem_bot + main_memory - 1;
    mem_min = mem_bot;
    mem_max = mem_top;

    /* Allocate many of our big arrays. */

    buffer = xmalloc_array(UnicodeScalar, buf_size);
    nest = xmalloc_array(list_state_record, nest_size);
    save_stack = xmalloc_array(memory_word, save_size);
    input_stack = xmalloc_array(input_state_t, stack_size);
    input_file = xmalloc_array(UFILE *, max_in_open);
    line_stack = xmalloc_array(integer, max_in_open);
    eof_seen = xmalloc_array(boolean, max_in_open);
    grp_stack = xmalloc_array(save_pointer, max_in_open);
    if_stack = xmalloc_array(int32_t, max_in_open);
    source_filename_stack = xmalloc_array(str_number, max_in_open);
    full_source_filename_stack = xmalloc_array(str_number, max_in_open);
    param_stack = xmalloc_array(int32_t, param_size);
    dvi_buf = xmalloc_array(eight_bits, dvi_buf_size);
    hyph_word = xmalloc_array(str_number, hyph_size);
    hyph_list = xmalloc_array(int32_t, hyph_size);
    hyph_link = xmalloc_array(hyph_pointer, hyph_size);

    if (in_initex_mode) {
        yzmem = xmalloc_array(memory_word, mem_top - mem_bot + 1);
        zmem = yzmem - mem_bot;
        eqtb_top = 10053470L /*eqtb_size */  + hash_extra;
        if (hash_extra == 0)
            hash_top = 2252239L /*undefined_control_sequence */ ;
        else
            hash_top = eqtb_top;
        yhash = xmalloc_array(two_halves, 1 + hash_top - hash_offset);
        hash = yhash - hash_offset;
        hash[2228226L /*hash_base */ ].v.LH = 0;
        hash[2228226L /*hash_base */ ].v.RH = 0;
        {
            register integer for_end;
            hash_used = 2228227L /*hash_base 1 */ ;
            for_end = hash_top;
            if (hash_used <= for_end)
                do
                    hash[hash_used] = hash[2228226L /*hash_base */ ];
                while (hash_used++ < for_end);
        }
        zeqtb = xmalloc_array(memory_word, eqtb_top);
        eqtb = zeqtb;
        str_start = xmalloc_array(pool_pointer, max_strings);
        str_pool = xmalloc_array(packed_UTF16_code, pool_size);
        font_info = xmalloc_array(fmemory_word, font_mem_size);
    }

    /* Sanity-check various invariants. */

    history = HISTORY_FATAL_ERROR;
    bad = 0;

    if ((half_error_line < 30) || (half_error_line > error_line - 15))
        bad = 1;
    if (max_print_line < 60)
        bad = 2;
    if (dvi_buf_size % 8 != 0)
        bad = 3;
    if (mem_bot + 1100 > mem_top)
        bad = 4;
    if (8501 /*hash_prime */  > 15000 /*hash_size */ )
        bad = 5;
    if (max_in_open >= 128)
        bad = 6;
    if (mem_top < 267)
        bad = 7;
    if ((mem_min != mem_bot) || (mem_max != mem_top))
        bad = 10;
    if ((mem_min > mem_bot) || (mem_max < mem_top))
        bad = 10;
    if ((-268435455L > 0) || (1073741823L < 1073741823L))
        bad = 12;
    if ((mem_bot - sup_main_memory < -268435455L) || (mem_top + sup_main_memory >= 1073741823L))
        bad = 14;
    if ((9000 /*max_font_max */  < -268435455L) || (9000 /*max_font_max */  > 1073741823L))
        bad = 15;
    if (font_max > 9000 /*font_base 9000 */ )
        bad = 16;
    if ((save_size > 1073741823L) || (max_strings > 1073741823L))
        bad = 17;
    if (buf_size > 1073741823L)
        bad = 18;
    if (43607901L /*cs_token_flag 10053470 */  + hash_extra > 1073741823L)
        bad = 21;
    if ((hash_offset < 0) || (hash_offset > 2228226L /*hash_base */ ))
        bad = 42;
    if (format_default_length > INTEGER_MAX)
        bad = 31;
    if (2 * 1073741823L < mem_top - mem_min)
        bad = 41;

    if (bad > 0)
	_tt_abort ("failed internal consistency check #%d", bad);

    /* OK, ready to keep on initializing. */

    initialize_more_variables();

    if (in_initex_mode) {
        if (!get_strings_started())
            return history;
        initialize_primitives();
        init_str_ptr = str_ptr;
        init_pool_ptr = pool_ptr;
        get_date_and_time(&(eqtb[8938760L /*int_base 20 */ ].cint),
			  &(eqtb[8938761L /*int_base 21 */ ].cint),
			  &(eqtb[8938762L /*int_base 22 */ ].cint),
			  &(eqtb[8938763L /*int_base 23 */ ].cint));
    }

    /*55:*/
    selector = SELECTOR_TERM_ONLY;
    tally = 0;
    term_offset = 0;
    file_offset = 0;
    job_name = 0;
    name_in_progress = false;
    log_opened = false;
    output_file_name = 0;
    output_file_extension = 66151L /*".xdv"*/;
    input_ptr = 0;
    max_in_stack = 0;
    source_filename_stack[0] = 0;
    full_source_filename_stack[0] = 0;
    in_open = 0;
    open_parens = 0;
    max_buf_stack = 0;
    grp_stack[0] = 0;
    if_stack[0] = -268435455L;
    param_ptr = 0;
    max_param_stack = 0;

    first = buf_size;
    do {
	buffer[first] = 0;
	first--;
    } while (first != 0);

    scanner_status = 0 /*normal*/;
    warning_index = -268435455L;
    first = 1;
    cur_input.state = 33 /*new_line*/;
    cur_input.start = 1;
    cur_input.index = 0;
    line = 0;
    cur_input.name = 0;
    force_eof = false;
    align_state = 1000000L;

    if (!init_terminal(input_file_name))
	return history;

    cur_input.limit = last;
    first = last + 1;

    if ((etex_p || buffer[cur_input.loc] == 42 /*"*"*/) && format_ident == 66676L /*" (INITEX)"*/) {
	no_new_control_sequence = false;
	primitive(66716L /*"XeTeXpicfile"*/, 59 /*extension*/, 41 /*pic_file_code*/);
	primitive(66717L /*"XeTeXpdffile"*/, 59 /*extension*/, 42 /*pdf_file_code*/);
	primitive(66718L /*"XeTeXglyph"*/, 59 /*extension*/, 43 /*glyph_code*/);
	primitive(66719L /*"XeTeXlinebreaklocale"*/, 59 /*extension*/, 46 /*XeTeX_linebreak_locale_extension_code*/);
	primitive(66720L /*"XeTeXinterchartoks"*/, 73 /*assign_toks*/, 2252782L /*XeTeX_inter_char_loc*/);
	primitive(66721L /*"pdfsavepos"*/, 59 /*extension*/, 6 /*pdftex_first_extension_code 0*/);
	primitive(66779L /*"lastnodetype"*/, 71 /*last_item*/, 3 /*last_node_type_code*/);
	primitive(66780L /*"eTeXversion"*/, 71 /*last_item*/, 6 /*eTeX_version_code*/);
	primitive(66781L /*"eTeXrevision"*/, 110 /*convert*/, 5 /*eTeX_revision_code*/);
	primitive(66782L /*"XeTeXversion"*/, 71 /*last_item*/, 14 /*XeTeX_version_code*/);
	primitive(66783L /*"XeTeXrevision"*/, 110 /*convert*/, 6 /*XeTeX_revision_code*/);
	primitive(66784L /*"XeTeXcountglyphs"*/, 71 /*last_item*/, 15 /*XeTeX_count_glyphs_code*/);
	primitive(66785L /*"XeTeXcountvariations"*/, 71 /*last_item*/, 16 /*XeTeX_count_variations_code*/);
	primitive(66786L /*"XeTeXvariation"*/, 71 /*last_item*/, 17 /*XeTeX_variation_code*/);
	primitive(66787L /*"XeTeXfindvariationbyname"*/, 71 /*last_item*/, 18 /*XeTeX_find_variation_by_name_code*/);
	primitive(66788L /*"XeTeXvariationmin"*/, 71 /*last_item*/, 19 /*XeTeX_variation_min_code*/);
	primitive(66789L /*"XeTeXvariationmax"*/, 71 /*last_item*/, 20 /*XeTeX_variation_max_code*/);
	primitive(66790L /*"XeTeXvariationdefault"*/, 71 /*last_item*/, 21 /*XeTeX_variation_default_code*/);
	primitive(66791L /*"XeTeXcountfeatures"*/, 71 /*last_item*/, 22 /*XeTeX_count_features_code*/);
	primitive(66792L /*"XeTeXfeaturecode"*/, 71 /*last_item*/, 23 /*XeTeX_feature_code_code*/);
	primitive(66793L /*"XeTeXfindfeaturebyname"*/, 71 /*last_item*/, 24 /*XeTeX_find_feature_by_name_code*/);
	primitive(66794L /*"XeTeXisexclusivefeature"*/, 71 /*last_item*/, 25 /*XeTeX_is_exclusive_feature_code*/);
	primitive(66795L /*"XeTeXcountselectors"*/, 71 /*last_item*/, 26 /*XeTeX_count_selectors_code*/);
	primitive(66796L /*"XeTeXselectorcode"*/, 71 /*last_item*/, 27 /*XeTeX_selector_code_code*/);
	primitive(66797L /*"XeTeXfindselectorbyname"*/, 71 /*last_item*/, 28 /*XeTeX_find_selector_by_name_code*/);
	primitive(66798L /*"XeTeXisdefaultselector"*/, 71 /*last_item*/, 29 /*XeTeX_is_default_selector_code*/);
	primitive(66799L /*"XeTeXvariationname"*/, 110 /*convert*/, 7 /*XeTeX_variation_name_code*/);
	primitive(66800L /*"XeTeXfeaturename"*/, 110 /*convert*/, XeTeX_feature_name);
	primitive(66801L /*"XeTeXselectorname"*/, 110 /*convert*/, XeTeX_selector_name);
	primitive(66802L /*"XeTeXOTcountscripts"*/, 71 /*last_item*/, 30 /*XeTeX_OT_count_scripts_code*/);
	primitive(66803L /*"XeTeXOTcountlanguages"*/, 71 /*last_item*/, 31 /*XeTeX_OT_count_languages_code*/);
	primitive(66804L /*"XeTeXOTcountfeatures"*/, 71 /*last_item*/, 32 /*XeTeX_OT_count_features_code*/);
	primitive(66805L /*"XeTeXOTscripttag"*/, 71 /*last_item*/, 33 /*XeTeX_OT_script_code*/);
	primitive(66806L /*"XeTeXOTlanguagetag"*/, 71 /*last_item*/, 34 /*XeTeX_OT_language_code*/);
	primitive(66807L /*"XeTeXOTfeaturetag"*/, 71 /*last_item*/, 35 /*XeTeX_OT_feature_code*/);
	primitive(66808L /*"XeTeXcharglyph"*/, 71 /*last_item*/, 36 /*XeTeX_map_char_to_glyph_code*/);
	primitive(66809L /*"XeTeXglyphindex"*/, 71 /*last_item*/, 37 /*XeTeX_glyph_index_code*/);
	primitive(66810L /*"XeTeXglyphbounds"*/, 71 /*last_item*/, 47 /*XeTeX_glyph_bounds_code*/);
	primitive(66811L /*"XeTeXglyphname"*/, 110 /*convert*/, 10 /*XeTeX_glyph_name_code*/);
	primitive(66812L /*"XeTeXfonttype"*/, 71 /*last_item*/, 38 /*XeTeX_font_type_code*/);
	primitive(66813L /*"XeTeXfirstfontchar"*/, 71 /*last_item*/, 39 /*XeTeX_first_char_code*/);
	primitive(66814L /*"XeTeXlastfontchar"*/, 71 /*last_item*/, 40 /*XeTeX_last_char_code*/);
	primitive(66815L /*"pdflastxpos"*/, 71 /*last_item*/, 41 /*pdf_last_x_pos_code*/);
	primitive(66816L /*"pdflastypos"*/, 71 /*last_item*/, 42 /*pdf_last_y_pos_code*/);
	primitive(66092L /*"strcmp"*/, 110 /*convert*/, 43 /*pdf_strcmp_code*/);
	primitive(66093L /*"mdfivesum"*/, 110 /*convert*/, 44 /*pdf_mdfive_sum_code*/);
	primitive(66014L /*"shellescape"*/, 71 /*last_item*/, 45 /*pdf_shell_escape_code*/);
	primitive(66817L /*"XeTeXpdfpagecount"*/, 71 /*last_item*/, 46 /*XeTeX_pdf_page_count_code*/);
	primitive(66830L /*"everyeof"*/, 73 /*assign_toks*/, 2252781L /*every_eof_loc*/);
	primitive(66831L /*"tracingassigns"*/, 74 /*assign_int*/, 8938798L /*int_base 58*/);
	primitive(66832L /*"tracinggroups"*/, 74 /*assign_int*/, 8938799L /*int_base 59*/);
	primitive(66833L /*"tracingifs"*/, 74 /*assign_int*/, 8938800L /*int_base 60*/);
	primitive(66834L /*"tracingscantokens"*/, 74 /*assign_int*/, 8938801L /*int_base 61*/);
	primitive(66835L /*"tracingnesting"*/, 74 /*assign_int*/, 8938802L /*int_base 62*/);
	primitive(66836L /*"predisplaydirection"*/, 74 /*assign_int*/, 8938803L /*int_base 63*/);
	primitive(66837L /*"lastlinefit"*/, 74 /*assign_int*/, 8938804L /*int_base 64*/);
	primitive(66838L /*"savingvdiscards"*/, 74 /*assign_int*/, 8938805L /*int_base 65*/);
	primitive(66839L /*"savinghyphcodes"*/, 74 /*assign_int*/, 8938806L /*int_base 66*/);
	primitive(66853L /*"currentgrouplevel"*/, 71 /*last_item*/, 7 /*current_group_level_code*/);
	primitive(66854L /*"currentgrouptype"*/, 71 /*last_item*/, 8 /*current_group_type_code*/);
	primitive(66855L /*"currentiflevel"*/, 71 /*last_item*/, 9 /*current_if_level_code*/);
	primitive(66856L /*"currentiftype"*/, 71 /*last_item*/, 10 /*current_if_type_code*/);
	primitive(66857L /*"currentifbranch"*/, 71 /*last_item*/, 11 /*current_if_branch_code*/);
	primitive(66858L /*"fontcharwd"*/, 71 /*last_item*/, 48 /*font_char_wd_code*/);
	primitive(66859L /*"fontcharht"*/, 71 /*last_item*/, 49 /*font_char_ht_code*/);
	primitive(66860L /*"fontchardp"*/, 71 /*last_item*/, 50 /*font_char_dp_code*/);
	primitive(66861L /*"fontcharic"*/, 71 /*last_item*/, 51 /*font_char_ic_code*/);
	primitive(66862L /*"parshapelength"*/, 71 /*last_item*/, 52 /*par_shape_length_code*/);
	primitive(66863L /*"parshapeindent"*/, 71 /*last_item*/, 53 /*par_shape_indent_code*/);
	primitive(66864L /*"parshapedimen"*/, 71 /*last_item*/, 54 /*par_shape_dimen_code*/);
	primitive(66865L /*"showgroups"*/, 19 /*xray*/, 4 /*show_groups*/);
	primitive(66867L /*"showtokens"*/, 19 /*xray*/, 5 /*show_tokens*/);
	primitive(66868L /*"unexpanded"*/, 111 /*the*/, 1);
	primitive(66869L /*"detokenize"*/, 111 /*the*/, 5 /*show_tokens*/);
	primitive(66870L /*"showifs"*/, 19 /*xray*/, 6 /*show_ifs*/);
	primitive(66874L /*"interactionmode"*/, 83 /*set_page_int*/, 2);
	primitive(66250L /*"middle"*/, 49 /*left_right*/, 1);
	primitive(66878L /*"suppressfontnotfounderror"*/, 74 /*assign_int*/, 8938807L /*int_base 67*/);
	primitive(66879L /*"TeXXeTstate"*/, 74 /*assign_int*/, 8938811L /*eTeX_state_base 0*/);
	primitive(66880L /*"XeTeXupwardsmode"*/, 74 /*assign_int*/, 8938813L /*eTeX_state_base 2*/);
	primitive(66881L /*"XeTeXuseglyphmetrics"*/, 74 /*assign_int*/, 8938814L /*eTeX_state_base 3*/);
	primitive(66882L /*"XeTeXinterchartokenstate"*/, 74 /*assign_int*/, 8938815L /*eTeX_state_base 4*/);
	primitive(66883L /*"XeTeXdashbreakstate"*/, 74 /*assign_int*/, 8938812L /*eTeX_state_base 1*/);
	primitive(66884L /*"XeTeXinputnormalization"*/, 74 /*assign_int*/, 8938816L /*eTeX_state_base 5*/);
	primitive(66885L /*"XeTeXtracingfonts"*/, 74 /*assign_int*/, 8938819L /*eTeX_state_base 8*/);
	primitive(66886L /*"XeTeXinterwordspaceshaping"*/, 74 /*assign_int*/, 8938820L /*eTeX_state_base 9*/);
	primitive(66887L /*"XeTeXgenerateactualtext"*/, 74 /*assign_int*/, 8938821L /*eTeX_state_base 10*/);
	primitive(66888L /*"XeTeXhyphenatablelength"*/, 74 /*assign_int*/, 8938822L /*eTeX_state_base 11*/);
	primitive(66722L /*"XeTeXinputencoding"*/, 59 /*extension*/, 44 /*XeTeX_input_encoding_extension_code*/);
	primitive(66723L /*"XeTeXdefaultencoding"*/, 59 /*extension*/, 45 /*XeTeX_default_encoding_extension_code*/);
	primitive(66889L /*"beginL"*/, 33 /*valign*/, 6 /*begin_L_code*/);
	primitive(66890L /*"endL"*/, 33 /*valign*/, 7 /*end_L_code*/);
	primitive(66891L /*"beginR"*/, 33 /*valign*/, 10 /*begin_R_code*/);
	primitive(66892L /*"endR"*/, 33 /*valign*/, 11 /*end_R_code*/);
	primitive(66901L /*"scantokens"*/, 106 /*input*/, 2);
	primitive(66903L /*"readline"*/, 98 /*read_to_cs*/, 1);
	primitive(66130L /*"unless"*/, 104 /*expand_after*/, 1);
	primitive(66904L /*"ifdefined"*/, 107 /*if_test*/, 17 /*if_def_code*/);
	primitive(66905L /*"ifcsname"*/, 107 /*if_test*/, 18 /*if_cs_code*/);
	primitive(66906L /*"iffontchar"*/, 107 /*if_test*/, 19 /*if_font_char_code*/);
	primitive(66907L /*"ifincsname"*/, 107 /*if_test*/, 20 /*if_in_csname_code*/);
	primitive(66586L /*"protected"*/, 95 /*prefix*/, 8);
	primitive(66913L /*"numexpr"*/, 71 /*last_item*/, 59 /*eTeX_expr -0 0*/);
	primitive(66914L /*"dimexpr"*/, 71 /*last_item*/, 60 /*eTeX_expr -0 1*/);
	primitive(66915L /*"glueexpr"*/, 71 /*last_item*/, 61 /*eTeX_expr -0 2*/);
	primitive(66916L /*"muexpr"*/, 71 /*last_item*/, 62 /*eTeX_expr -0 3*/);
	primitive(66920L /*"gluestretchorder"*/, 71 /*last_item*/, 12 /*glue_stretch_order_code*/);
	primitive(66921L /*"glueshrinkorder"*/, 71 /*last_item*/, 13 /*glue_shrink_order_code*/);
	primitive(66922L /*"gluestretch"*/, 71 /*last_item*/, 55 /*glue_stretch_code*/);
	primitive(66923L /*"glueshrink"*/, 71 /*last_item*/, 56 /*glue_shrink_code*/);
	primitive(66924L /*"mutoglue"*/, 71 /*last_item*/, 57 /*mu_to_glue_code*/);
	primitive(66925L /*"gluetomu"*/, 71 /*last_item*/, 58 /*glue_to_mu_code*/);
	primitive(66926L /*"marks"*/, 18 /*mark*/, 5);
	primitive(66927L /*"topmarks"*/, 112 /*top_bot_mark*/, 5 /*top_mark_code 5*/);
	primitive(66928L /*"firstmarks"*/, 112 /*top_bot_mark*/, 6 /*first_mark_code 5*/);
	primitive(66929L /*"botmarks"*/, 112 /*top_bot_mark*/, 7 /*bot_mark_code 5*/);
	primitive(66930L /*"splitfirstmarks"*/, 112 /*top_bot_mark*/, 8 /*split_first_mark_code 5*/);
	primitive(66931L /*"splitbotmarks"*/, 112 /*top_bot_mark*/, 9 /*split_bot_mark_code 5*/);
	primitive(66936L /*"pagediscards"*/, 24 /*un_vbox*/, 2 /*last_box_code*/);
	primitive(66937L /*"splitdiscards"*/, 24 /*un_vbox*/, 3 /*vsplit_code*/);
	primitive(66938L /*"interlinepenalties"*/, 85 /*set_shape*/, 2253039L /*inter_line_penalties_loc*/);
	primitive(66939L /*"clubpenalties"*/, 85 /*set_shape*/, 2253040L /*club_penalties_loc*/);
	primitive(66940L /*"widowpenalties"*/, 85 /*set_shape*/, 2253041L /*widow_penalties_loc*/);
	primitive(66941L /*"displaywidowpenalties"*/, 85 /*set_shape*/, 2253042L /*display_widow_penalties_loc*/);

	if (buffer[cur_input.loc] == 42 /*"*"*/)
	    cur_input.loc++;

	eTeX_mode = 1;
	max_reg_num = 32767;
	max_reg_help_line = 66933L /*"A register number must be between 0 and 32767."*/;
    }

    if (!no_new_control_sequence)
	no_new_control_sequence = true;
    else if (format_ident == 0 || buffer[cur_input.loc] == 38 /*"&"*/ || dump_line) {
	if (format_ident != 0)
	    initialize_more_variables();

	if (!load_fmt_file())
	    return history;

	eqtb = zeqtb;

	while (cur_input.loc < cur_input.limit && buffer[cur_input.loc] == 32 /*" "*/)
	    cur_input.loc++;
    }

    if (eTeX_mode == 1) {
	char *msg = "entering extended mode\n";
	ttstub_output_write (rust_stdout, msg, strlen (msg));
    }

    if (eqtb[8938788L /*int_base 48*/].cint < 0 || eqtb[8938788L /*int_base 48*/].cint > 255)
	cur_input.limit--;
    else
	buffer[cur_input.limit] = eqtb[8938788L /*int_base 48*/].cint;

    if (mltex_enabled_p) {
	char *msg = "MLTeX v2.2 enabled\n";
	ttstub_output_write (rust_stdout, msg, strlen (msg));
    }

    get_date_and_time(&(eqtb[8938760L /*int_base 20*/].cint),
		      &(eqtb[8938761L /*int_base 21*/].cint),
		      &(eqtb[8938762L /*int_base 22*/].cint),
		      &(eqtb[8938763L /*int_base 23*/].cint));

    if (trie_not_ready) {
	trie_trl = xmalloc_array(trie_pointer, trie_size);
	trie_tro = xmalloc_array(trie_pointer, trie_size);
	trie_trc = xmalloc_array(uint16_t, trie_size);
	trie_c = xmalloc_array(packed_UTF16_code, trie_size);
	trie_o = xmalloc_array(trie_opcode, trie_size);
	trie_l = xmalloc_array(trie_pointer, trie_size);
	trie_r = xmalloc_array(trie_pointer, trie_size);
	trie_hash = xmalloc_array(trie_pointer, trie_size);
	trie_taken = xmalloc_array(boolean, trie_size);
	trie_l[0] = 0;
	trie_c[0] = 0;
	trie_ptr = 0;
	trie_r[0] = 0;
	hyph_start = 0;
	font_mapping = xmalloc_array(void *, font_max);
	font_layout_engine = xmalloc_array(void *, font_max);
	font_flags = xmalloc_array(char, font_max);
	font_letter_space = xmalloc_array(scaled, font_max);
	font_check = xmalloc_array(four_quarters, font_max);
	font_size = xmalloc_array(scaled, font_max);
	font_dsize = xmalloc_array(scaled, font_max);
	font_params = xmalloc_array(font_index, font_max);
	font_name = xmalloc_array(str_number, font_max);
	font_area = xmalloc_array(str_number, font_max);
	font_bc = xmalloc_array(UTF16_code, font_max);
	font_ec = xmalloc_array(UTF16_code, font_max);
	font_glue = xmalloc_array(int32_t, font_max);
	hyphen_char = xmalloc_array(integer, font_max);
	skew_char = xmalloc_array(integer, font_max);
	bchar_label = xmalloc_array(font_index, font_max);
	font_bchar = xmalloc_array(nine_bits, font_max);
	font_false_bchar = xmalloc_array(nine_bits, font_max);
	char_base = xmalloc_array(integer, font_max);
	width_base = xmalloc_array(integer, font_max);
	height_base = xmalloc_array(integer, font_max);
	depth_base = xmalloc_array(integer, font_max);
	italic_base = xmalloc_array(integer, font_max);
	lig_kern_base = xmalloc_array(integer, font_max);
	kern_base = xmalloc_array(integer, font_max);
	exten_base = xmalloc_array(integer, font_max);
	param_base = xmalloc_array(integer, font_max);
	font_ptr = 0 /*font_base*/;
	fmem_ptr = 7;
	font_name[0 /*font_base*/] = 66159L /*"nullfont"*/;
	font_area[0 /*font_base*/] = 65622L /*""*/;
	hyphen_char[0 /*font_base*/] = 45 /*"-"*/;
	skew_char[0 /*font_base*/] = -1;
	bchar_label[0 /*font_base*/] = 0 /*non_address*/;
	font_bchar[0 /*font_base*/] = 65536L /*too_big_char*/;
	font_false_bchar[0 /*font_base*/] = 65536L /*too_big_char*/;
	font_bc[0 /*font_base*/] = 1;
	font_ec[0 /*font_base*/] = 0;
	font_size[0 /*font_base*/] = 0;
	font_dsize[0 /*font_base*/] = 0;
	char_base[0 /*font_base*/] = 0;
	width_base[0 /*font_base*/] = 0;
	height_base[0 /*font_base*/] = 0;
	depth_base[0 /*font_base*/] = 0;
	italic_base[0 /*font_base*/] = 0;
	lig_kern_base[0 /*font_base*/] = 0;
	kern_base[0 /*font_base*/] = 0;
	exten_base[0 /*font_base*/] = 0;
	font_glue[0 /*font_base*/] = -268435455L;
	font_params[0 /*font_base*/] = 7;
	font_mapping[0 /*font_base*/] = 0;
	param_base[0 /*font_base*/] = -1;

	for (font_k = 0; font_k <= 6; font_k++)
	    font_info[font_k].cint = 0;
    }

    font_used = xmalloc_array(boolean, font_max);
    for (font_k = 0; font_k <= font_max; font_k++)
	font_used[font_k] = false;

    magic_offset = str_start[(66282L /*math_spacing*/) - 65536L] - 9 * 16 /*ord_noad*//*:794*/;

    if (interaction == 0 /*batch_mode*/)
	selector = SELECTOR_NO_PRINT;
    else
	selector = SELECTOR_TERM_ONLY; /*:79*/

    /* OK, we are finally ready to go!
     *
     * Below is the key line that looks at the "first line" that we've
     * synthesized. If it doesn't begin with a control character, we pretend
     * that the user has essentially written "\input ..." */

    if (cur_input.loc < cur_input.limit
	&& eqtb[2254068L /*cat_code_base*/ + buffer[cur_input.loc]].hh.v.RH != 0 /*escape*/)
	start_input();

    history = HISTORY_SPOTLESS;
    synctex_init_command();
    main_control();
    final_cleanup();
    close_files_and_terminate();
    return history;
}


/* These functions don't belong here except that we want to reuse the
 * infrastructure for error handling with longjmp() and _tt_abort().
 */

int
dvipdfmx_simple_main(char *dviname, char *pdfname)
{
    extern int dvipdfmx_main(int argc, char *argv[]);

    char *argv[] = { "dvipdfmx", "-o", pdfname, dviname };

    if (setjmp (jump_buffer))
	return 99;

    return dvipdfmx_main(4, argv);
}


int
bibtex_simple_main(char *aux_file_name)
{
    extern tt_history_t bibtex_main_body(const char *aux_file_name);

    if (setjmp (jump_buffer))
        return 99;

    return bibtex_main_body(aux_file_name);
}
