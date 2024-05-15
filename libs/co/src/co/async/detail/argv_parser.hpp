
#pragma once

#include <limits>

#if ! defined SIZE_MAX
#define SIZE_MAX (4294967295U)
#endif

// argv_parser implements VS2015 CRT's way of parsing cmdline.

template <typename Char>
class argv_parser {
public:
  argv_parser();
  ~argv_parser();
  argv_parser(const argv_parser& r);
  argv_parser(const Char* cmd_line);

  void operator=(const argv_parser& r);

  bool parse(const Char* cmd_line);

  int argc() const { return argc_; }
  Char** argv() { return argv_; }
  const Char** arg_starts() const { return arg_starts_; }

private:
  void copy_from(const argv_parser& r);
  void free_argv();
  void parse_cmdline(
    const Char*,
    Char **,
    Char *,
    int *,
    int *,
    const Char**);

private:
  int              argc_;
  Char**           argv_;
  size_t           argv_len_;
  const Char**     arg_starts_;
};


template class argv_parser<char>;
template class argv_parser<wchar_t>;

// ---

// Convertion.
template <typename DestChar, typename SourceChar>
static inline DestChar tchar_to_tchar(SourceChar ansi_char) {
  return static_cast<DestChar>(ansi_char);
}
static inline char wchar_to_char(wchar_t c) {
  return tchar_to_tchar<char, wchar_t>(c);
}
static inline wchar_t char_to_wchar(char c) {
  return tchar_to_tchar<wchar_t, char>(c);
}

template <typename DestChar, typename SourceChar>
static std::basic_string<DestChar> tstr_to_tstr(const std::basic_string<SourceChar>& str) {
  return std::basic_string<DestChar>(str.begin(), str.end());
}
static inline std::wstring str_to_wstr(const std::string& subject) {
  return tstr_to_tstr<wchar_t, char>(subject);
}
static inline std::string wstr_to_str(const std::wstring& subject) {
  return tstr_to_tstr<char, wchar_t>(subject);
}

template <typename C> static C char_to_tchar(char c) {
  return tchar_to_tchar<C, char>(c);
}
template <typename C> static C wchar_to_tchar(wchar_t c) {
  return tchar_to_tchar<C, wchar_t>(c);
}

template <typename C> static std::basic_string<C> str_to_tstr(
  const std::string& s)
{
  return tstr_to_tstr<C, char>(s);
}
template <typename C> static std::basic_string<C> wstr_to_tstr(
  const std::wstring& s)
{
  return tstr_to_tstr<C, wchar_t>(s);
}

// ---

template <typename Char>
argv_parser<Char>::argv_parser() {
  argv_len_ = 0;
  argc_ = 0;
  argv_ = 0;
}

template <typename Char>
argv_parser<Char>::~argv_parser() {
  free_argv();
  free(arg_starts_);
}

template <typename Char>
argv_parser<Char>::argv_parser(const argv_parser& r) {
  copy_from(r);
}

template <typename Char>
argv_parser<Char>::argv_parser(const Char* cmd_line) {
  if (!parse(cmd_line)) {
  }
}

template <typename Char>
void argv_parser<Char>::operator=(const argv_parser<Char>& r) {
  copy_from(r);
}

template <typename Char>
bool argv_parser<Char>::parse(const Char* cmd_line) {
  int numargs, numchars;
  parse_cmdline(cmd_line, 0, 0, &numargs, &numchars, 0);

  if (numargs >= (SIZE_MAX / sizeof(Char *)) ||
      numchars >= (SIZE_MAX / sizeof(Char)))
  {
    return false;
  }
  if (
    (numargs * sizeof(Char *) + numchars * sizeof(Char)) <
    (numchars * sizeof(Char)))
  {
    return false;
  }
  size_t argv_len = numargs * sizeof(Char *) + numchars * sizeof(Char);
  Char* p = (Char*)malloc(argv_len);
  if (p == NULL) {
    return false;
  }
  arg_starts_ = (const Char**)malloc(numargs*sizeof(Char*));
  if (arg_starts_ == NULL) {
    free_argv();
    return false;
  }
  /* store args and argv ptrs in just allocated block */
  parse_cmdline(
    cmd_line, (Char **)p,
    (Char*)(((char *)p) + numargs * sizeof(Char*)), &numargs, &numchars,
    arg_starts_);

  argv_len_ = argv_len;
  argc_ = numargs - 1;
  argv_ = (Char **)p;
  return true;
}

template <typename Char>
void argv_parser<Char>::copy_from(const argv_parser& r) {
  if ((argc_ = r.argc_)) {
    argv_ = (Char**)malloc(r.argv_len_);
    memcpy(argv_, r.argv_, r.argv_len_);
    argv_len_ = r.argv_len_;
  } else {
    argv_ = 0;
    argv_len_ = 0;
  }
}

template <typename Char>
void argv_parser<Char>::free_argv() {
  if (argv_) {
    free(argv_);
    argv_ = 0;
  }
}

#define NULCHAR      char_to_tchar<Char>('\0')
#define SPACECHAR    char_to_tchar<Char>(' ')
#define TABCHAR      char_to_tchar<Char>('\t')
#define DQUOTECHAR   char_to_tchar<Char>('\"')
#define SLASHCHAR    char_to_tchar<Char>('\\')

template <typename Char>
void argv_parser<Char>::parse_cmdline(
  const Char* cmdstart,
  Char **argv,
  Char *args,
  int *numargs,
  int *numchars,
  const Char** pstarts // Added by me.
  )
{
  const Char *p;
  unsigned char c;
  int inquote;                    /* 1 = inside quotes */
  int copychar;                   /* 1 = copy char to *args */
  unsigned numslash;              /* num of backslashes seen */

  *numchars = 0;
  *numargs = 1;                   /* the program name at least */

                                  /* first scan the program name, copy it, and count the bytes */
  p = cmdstart;
  if (argv)
    *argv++ = args;

  /* A quoted program name is handled here. The handling is much
  simpler than for other arguments. Basically, whatever lies
  between the leading double-quote and next one, or a terminal null
  character is simply accepted. Fancier handling is not required
  because the program name must be a legal NTFS/HPFS file name.
  Note that the double-quote characters are not copied, nor do they
  contribute to numchars. */
  inquote = 0;
  do {
    if (*p == DQUOTECHAR)
    {
      inquote = !inquote;
      c = (unsigned char)*p++;
      continue;
    }
    ++*numchars;
    if (args)
      *args++ = *p;

    c = (unsigned char)*p++;

  } while ((c != NULCHAR && (inquote || (c != SPACECHAR && c != TABCHAR))));

  if (c == NULCHAR) {
    p--;
  } else {
    if (args)
      *(args - 1) = NULCHAR;
  }

  inquote = 0;

  /* loop on each argument */
  for (;;) {
    if (*p) {
      while (*p == SPACECHAR || *p == TABCHAR)
        ++p;
    }

    // Added by me:
    if (pstarts) {
      *pstarts = p;
      ++pstarts;
    }

    if (*p == NULCHAR)
      break;              /* end of args */


                          /* scan an argument */
    if (argv)
      *argv++ = args;     /* store ptr to arg */
    ++*numargs;

    /* loop through scanning one argument */
    for (;;) {
      copychar = 1;
      /* Rules: 2N backslashes + " ==> N backslashes and begin/end quote
      2N+1 backslashes + " ==> N backslashes + literal "
      N backslashes ==> N backslashes */
      numslash = 0;
      while (*p == SLASHCHAR) {
        /* count number of backslashes for use below */
        ++p;
        ++numslash;
      }
      if (*p == DQUOTECHAR) {
        /* if 2N backslashes before, start/end quote, otherwise
        copy literally */
        if (numslash % 2 == 0) {
          if (inquote && p[1] == DQUOTECHAR) {
            p++;    /* Double quote inside quoted string */
          } else {    /* skip first quote char and copy second */
            copychar = 0;       /* don't copy quote */
            inquote = !inquote;
          }
        }
        numslash /= 2;          /* divide numslash by two */
      }

      /* copy slashes */
      while (numslash--) {
        if (args)
          *args++ = SLASHCHAR;
        ++*numchars;
      }

      /* if at end of arg, break loop */
      if (*p == NULCHAR || (!inquote && (*p == SPACECHAR || *p == TABCHAR)))
        break;

      /* copy character into argument */
      if (copychar) {
        if (args)
          *args++ = *p;
        ++*numchars;
      }
      ++p;
    }

    /* null-terminate the argument */

    if (args)
      *args++ = NULCHAR;          /* terminate string */
    ++*numchars;
  }

  /* We put one last argument in -- a null ptr */
  if (argv)
    *argv++ = NULL;
  ++*numargs;
}
