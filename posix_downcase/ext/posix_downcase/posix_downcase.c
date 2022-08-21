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
 * 2. original string is utf-8 or usascii encoded
 */

static rb_encoding *utf8_enc = NULL;
static rb_encoding *usascii_enc = NULL;

/*
 * Some old or poorly-configured versions of Oniguruma say 6 here, but that's
 * outrageous so we follow postgres instead (see postgres/src/common/wchar.c).
 */
#define MAX_ENC_LEN 4

/*
 * Downcases the supplied string according to US-ASCII semantics.
 * Returns a freshly allocated string (of length len) that the caller is
 * responsible for.
 */
static char *
posix_downcase_usascii(const char *string, size_t len)
{
  char *lower_str;
  size_t curr_char;
  unsigned char c;
  lower_str = (char *) malloc(len);
  for (curr_char = 0; curr_char < len; curr_char++)
    {
      c = string[curr_char];
      lower_str[curr_char] = (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
    }
  return lower_str;
}

/*
 * Downcases the supplied string according to UTF-8 POSIX locale semantics.
 * Returns a freshly allocated string that the caller is responsible for.
 */
static char *
posix_downcase_utf8(const char *string, size_t len)
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

VALUE
rb_utf8_str_posix_downcase(VALUE rb_string)
{
  char *result_cstr;
  rb_encoding *input_str_enc;
  VALUE result;
  char *input_str;
  size_t len;

  /* Check if the receiver is string. Raise `TypeError` otherwise */
  Check_Type(rb_string, T_STRING);

  /*
   * Only UTF-8 and US-ASCII encoding are supported at the moment for posix
   * downcase. Raises Encoding::CompatibilityError if encoding of reciever is
   * not UTF-8 or US-ASCII.
   */
  input_str_enc = rb_enc_get(rb_string);
  if (input_str_enc != utf8_enc && input_str_enc != usascii_enc)
    {
      rb_raise(rb_eEncCompatError,
               "Supplied argument of encoding %s, only UTF-8 and US-ASCII are supported",
               rb_enc_name(input_str_enc));
    }

  input_str = RSTRING_PTR(rb_string);
  len = RSTRING_LEN(rb_string);
  /* Downcase string based on rb_string encoding */
  if (input_str_enc == utf8_enc) {
    result_cstr = posix_downcase_utf8(input_str, len);
    result = rb_utf8_str_new_cstr(result_cstr);
  }
  else {
    /* This will be US-ASCII since we only support two encodings */
    result_cstr = posix_downcase_usascii(input_str, len);
    result = rb_usascii_str_new(result_cstr, len);
  }

  /* free resources */
  free(result_cstr);

  return result;
}

void
Init_posix_downcase(void)
{
  utf8_enc = rb_utf8_encoding();
  usascii_enc = rb_usascii_encoding();
  /*
   * Define `posix_downcase` instance method for ruby String.
   * Drop-in replacement of `downcase`.
   * Currently only supported for UTF-8 encoded strings.
  */
  rb_define_method(rb_cString, "posix_downcase", rb_utf8_str_posix_downcase, 0);
}
