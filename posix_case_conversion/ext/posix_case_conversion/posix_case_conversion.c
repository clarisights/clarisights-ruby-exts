#include <wctype.h>
#include <ruby.h>
#include <ruby/encoding.h>

/*
 * References
 * 1. postgres/src/backend/utils/adt/formatting.c
 *   > char * str_tolower(const char *buff, size_t nbytes, Oid collid)
 * 2. postgres/src/backend/utils/adt/pg_locale.c
 *   > size_t char2wchar(wchar_t *to, size_t tolen, const char *from, size_t fromlen, pg_locale_t locale)
 *   > size_t wchar2char(char *to, const wchar_t *from, size_t tolen, pg_locale_t locale)
 *
 * Pre-requisites
 * 1. locale is set (man 3 setlocale)
 * 2. original string is utf-8 encoded
 */

static rb_encoding *utf8_enc = NULL;

/*
 * Some old or poorly-configured versions of Oniguruma say 6 here, but that's
 * outrageous so we follow postgres instead (see postgres/src/common/wchar.c).
 */
#define MAX_ENC_LEN 4

/*
 * Downcases the supplied string according to POSIX locale semantics. Returns
 * a freshly allocated string that the caller is responsible for.
 */
static char *
posix_downcase(const char *string, size_t len)
{
  char *input_str, *output_str;
  size_t input_bytes, output_bytes;
  wchar_t *workspace;
  size_t curr_char;

  input_bytes = len;
  // The supplied string may not be NULL-terminated, which mbstowcs requires
  input_str = strndup(string, input_bytes);
  workspace = (wchar_t *) malloc((input_bytes + 1) * sizeof(wchar_t));
  /* convert multibyte to wide character */
  mbstowcs(workspace, input_str, input_bytes + 1);
  for (curr_char = 0; workspace[curr_char] != 0; curr_char++)
    {
      workspace[curr_char] = towlower(workspace[curr_char]);
    }
  /*
   * Make output large enough; case change might change number of bytes.
   * utf encoding max length belongs to [1, MAX_ENC_LEN]
   */
  output_bytes = curr_char * MAX_ENC_LEN + 1;
  output_str = (char *) malloc(output_bytes);
  /* convert wide character to multibyte */
  wcstombs(output_str, workspace, output_bytes);

  /* free resources */
  free(workspace);
  free(input_str);

  return output_str;
}

static VALUE
rb_utf8_str_posix_downcase(VALUE rb_string)
{
  char *result_cstr;
  VALUE result;

  /* Check if the receiver is string. Raise `TypeError` otherwise */
  Check_Type(rb_string, T_STRING);

  /*
   * Only UTF-8 encoding is supproted at the moment for posix downcase.
   * Raises Encoding::CompatibilityError if encoding of reciever is not UTF-8.
   */
  if (rb_enc_get(rb_string) != utf8_enc)
    {
      rb_raise(rb_eEncCompatError,
               "Supplied argument of encoding %s, only %s supported",
               rb_enc_name(rb_enc_get(rb_string)),
               rb_enc_name(utf8_enc));
    }

  result_cstr = posix_downcase(RSTRING_PTR(rb_string), RSTRING_LEN(rb_string));
  result = rb_enc_str_new_cstr(result_cstr, utf8_enc);

  /* free resources */
  free(result_cstr);

  return result;
}

void
Init_posix_case_conversion(void)
{
  utf8_enc = rb_utf8_encoding();
  /*
   * Define `posix_downcase` instance method for ruby String.
   * Drop-in replacement of `downcase`.
   * Currently only supported for UTF-8 encoded strings.
  */
  rb_define_method(rb_cString, "posix_downcase", rb_utf8_str_posix_downcase, 0);
}
