#include "optionparser.h"
namespace option {

void PrintUsageImplementation::LineWrapper::process (PrintUsageImplementation::IStringWriter& write, const TCHAR* data, int len)
{
    wrote_something = false;

    while (len > 0)
    {
    if (len <= width) // quick test that works because utf8width <= len (all wide chars have at least 2 bytes)
    {
        output(write, data, len);
        len = 0;
    }
    else // if (len > width)  it's possible (but not guaranteed) that utf8len > width
    {
        int utf8width = 0;
        int maxi = 0;
        while (maxi < len && utf8width < width)
        {
        int charbytes = 1;
        unsigned ch = static_cast <unsigned char> (data[maxi]);
        if (ch > 0xC1) // everything <= 0xC1 (yes, even 0xC1 itself) is not a valid UTF-8 start byte
        {
            // int __builtin_clz (unsigned int x)
            // Returns the number of leading 0-bits in x, starting at the most significant bit
            unsigned mask = static_cast <unsigned> (-1) >> __builtin_clz(ch ^ 0xff);
            ch = ch & mask; // mask out length bits, we don't verify their correctness
            while ((maxi + charbytes < len) && //
                ((static_cast <unsigned char> (data[maxi + charbytes]) ^ 0x80) <= 0x3F)) // while next byte is continuation byte
            {
            ch = (ch << 6) ^ static_cast <unsigned char> (data[maxi + charbytes]) ^ 0x80; // add continuation to char code
            ++charbytes;
            }
            // ch is the decoded unicode code point
            if (ch >= 0x1100 && isWideChar(ch)) // the test for 0x1100 is here to avoid the function call in the Latin case
            {
            if (utf8width + 2 > width)
                break;
            ++utf8width;
            }
        }
        ++utf8width;
        maxi += charbytes;
        }

        // data[maxi-1] is the last byte of the UTF-8 sequence of the last character that fits
        // onto the 1st line. If maxi == len, all characters fit on the line.

        if (maxi == len)
        {
        output(write, data, len);
        len = 0;
        }
        else // if (maxi < len)  at least 1 character (data[maxi] that is) doesn't fit on the line
        {
        int i;
        for (i = maxi; i >= 0; --i)
            if (data[i] == ' ')
            break;

        if (i >= 0)
        {
            output(write, data, i);
            data += i + 1;
            len -= i + 1;
        }
        else // did not find a space to split at => split before data[maxi]
        { // data[maxi] is always the beginning of a character, never a continuation byte
            output(write, data, maxi);
            data += maxi;
            len -= maxi;
        }
        }
    }
    }
    if (!wrote_something) // if we didn't already write something to make space in the buffer
    write_one_line(write); // write at most one line of actual output
}


bool Parser::workhorse(bool gnu, const Descriptor usage[], int numargs, const TCHAR ** args, Action& action,
                              bool single_minus_longopt, bool print_errors, int min_abbr_len)
{
  // protect against NULL pointer
  if (args == 0)
    numargs = 0;

  int nonops = 0;

  while (numargs != 0 && *args != 0)
  {
    const TCHAR * param = *args; // param can be --long-option, -srto or non-option argument

    // in POSIX mode the first non-option argument terminates the option list
    // a lone minus character is a non-option argument
    if (param[0] != '-' || param[1] == 0)
    {
      if (gnu)
      {
        ++nonops;
        ++args;
        if (numargs > 0)
          --numargs;
        continue;
      }
      else
        break;
    }

    // -- terminates the option list. The -- itself is skipped.
    if (param[1] == '-' && param[2] == 0)
    {
      shift(args, nonops);
      ++args;
      if (numargs > 0)
        --numargs;
      break;
    }

    bool handle_short_options;
    const TCHAR * longopt_name;
    if (param[1] == '-') // if --long-option
    {
      handle_short_options = false;
      longopt_name = param + 2;
    }
    else
    {
      handle_short_options = true;
      longopt_name = param + 1; //for testing a potential -long-option
    }

    bool try_single_minus_longopt = single_minus_longopt;
    bool have_more_args = (numargs > 1 || numargs < 0); // is referencing argv[1] valid?

    do // loop over short options in group, for long options the body is executed only once
    {
      int idx = 0;

      const TCHAR* optarg = 0;

      /******************** long option **********************/
      if (handle_short_options == false || try_single_minus_longopt)
      {
        idx = 0;
        while (usage[idx].longopt != 0 && !streq(usage[idx].longopt, longopt_name))
          ++idx;

        if (usage[idx].longopt == 0 && min_abbr_len > 0) // if we should try to match abbreviated long options
        {
          int i1 = 0;
          while (usage[i1].longopt != 0 && !streqabbr(usage[i1].longopt, longopt_name, min_abbr_len))
            ++i1;
          if (usage[i1].longopt != 0)
          { // now test if the match is unambiguous by checking for another match
            int i2 = i1 + 1;
            while (usage[i2].longopt != 0 && !streqabbr(usage[i2].longopt, longopt_name, min_abbr_len))
              ++i2;

            if (usage[i2].longopt == 0) // if there was no second match it's unambiguous, so accept i1 as idx
              idx = i1;
          }
        }

        // if we found something, disable handle_short_options (only relevant if single_minus_longopt)
        if (usage[idx].longopt != 0)
          handle_short_options = false;

        try_single_minus_longopt = false; // prevent looking for longopt in the middle of shortopt group

        optarg = longopt_name;
        while (*optarg != 0 && *optarg != '=')
          ++optarg;
        if (*optarg == '=') // attached argument
          ++optarg;
        else
          // possibly detached argument
          optarg = (have_more_args ? args[1] : 0);
      }

      /************************ short option ***********************************/
      if (handle_short_options)
      {
        if (*++param == 0) // point at the 1st/next option character
          break; // end of short option group

        idx = 0;
        while (usage[idx].shortopt != 0 && !instr(*param, usage[idx].shortopt))
          ++idx;

        if (param[1] == 0) // if the potential argument is separate
          optarg = (have_more_args ? args[1] : 0);
        else
          // if the potential argument is attached
          optarg = param + 1;
      }

      const Descriptor* descriptor = &usage[idx];

      if (descriptor->shortopt == 0) /**************  unknown option ********************/
      {
        // look for dummy entry (shortopt == "" and longopt == "") to use as Descriptor for unknown options
        idx = 0;
        while (usage[idx].shortopt != 0 && (usage[idx].shortopt[0] != 0 || usage[idx].longopt[0] != 0))
          ++idx;
        descriptor = (usage[idx].shortopt == 0 ? 0 : &usage[idx]);
      }

      if (descriptor != 0)
      {
        Option option(descriptor, param, optarg);
        switch (descriptor->check_arg(option, print_errors))
        {
          case ARG_ILLEGAL:
            return false; // fatal
          case ARG_OK:
            // skip one element of the argument vector, if it's a separated argument
            if (optarg != 0 && have_more_args && optarg == args[1])
            {
              shift(args, nonops);
              if (numargs > 0)
                --numargs;
              ++args;
            }

            // No further short options are possible after an argument
            handle_short_options = false;

            break;
          case ARG_IGNORE:
          case ARG_NONE:
            option.arg = 0;
            break;
        }

        if (!action.perform(option))
          return false;
      }

    } while (handle_short_options);

    shift(args, nonops);
    ++args;
    if (numargs > 0)
      --numargs;

  } // while

  if (numargs > 0 && *args == 0) // It's a bug in the caller if numargs is greater than the actual number
    numargs = 0; // of arguments, but as a service to the user we fix this if we spot it.

  if (numargs < 0) // if we don't know the number of remaining non-option arguments
  { // we need to count them
    numargs = 0;
    while (args[numargs] != 0)
      ++numargs;
  }

  return action.finished(numargs + nonops, args - nonops);
}


void PrintUsageImplementation::printUsage(IStringWriter& write, const Descriptor usage[], int width, //
                         int last_column_min_percent, int last_column_own_line_max_percent)
  {
    if (width < 1) // protect against nonsense values
      width = 80;

    if (width > 10000) // protect against overflow in the following computation
      width = 10000;

    int last_column_min_width = ((width * last_column_min_percent) + 50) / 100;
    int last_column_own_line_max_width = ((width * last_column_own_line_max_percent) + 50) / 100;
    if (last_column_own_line_max_width == 0)
      last_column_own_line_max_width = 1;

    LinePartIterator part(usage);
    while (part.nextTable())
    {

      /***************** Determine column widths *******************************/

      const int maxcolumns = 8; // 8 columns are enough for everyone
      int col_width[maxcolumns];
      int lastcolumn;
      int leftwidth;
      int overlong_column_threshold = 10000;
      do
      {
        lastcolumn = 0;
        for (int i = 0; i < maxcolumns; ++i)
          col_width[i] = 0;

        part.restartTable();
        while (part.nextRow())
        {
          while (part.next())
          {
            if (part.column() < maxcolumns)
            {
              upmax(lastcolumn, part.column());
              if (part.screenLength() < overlong_column_threshold)
                // We don't let rows that don't use table separators (\t or \v) influence
                // the width of column 0. This allows the user to interject section headers
                // or explanatory paragraphs that do not participate in the table layout.
                if (part.column() > 0 || part.line() > 0 || part.data()[part.length()] == '\t'
                    || part.data()[part.length()] == '\v')
                  upmax(col_width[part.column()], part.screenLength());
            }
          }
        }

        /*
         * If the last column doesn't fit on the same
         * line as the other columns, we can fix that by starting it on its own line.
         * However we can't do this for any of the columns 0..lastcolumn-1.
         * If their sum exceeds the maximum width we try to fix this by iteratively
         * ignoring the widest line parts in the width determination until
         * we arrive at a series of column widths that fit into one line.
         * The result is a layout where everything is nicely formatted
         * except for a few overlong fragments.
         * */

        leftwidth = 0;
        overlong_column_threshold = 0;
        for (int i = 0; i < lastcolumn; ++i)
        {
          leftwidth += col_width[i];
          upmax(overlong_column_threshold, col_width[i]);
        }

      } while (leftwidth > width);

      /**************** Determine tab stops and last column handling **********************/

      int tabstop[maxcolumns];
      tabstop[0] = 0;
      for (int i = 1; i < maxcolumns; ++i)
        tabstop[i] = tabstop[i - 1] + col_width[i - 1];

      int rightwidth = width - tabstop[lastcolumn];
      bool print_last_column_on_own_line = false;
      if (rightwidth < last_column_min_width && rightwidth < col_width[lastcolumn])
      {
        print_last_column_on_own_line = true;
        rightwidth = last_column_own_line_max_width;
      }

      // If lastcolumn == 0 we must disable print_last_column_on_own_line because
      // otherwise 2 copies of the last (and only) column would be output.
      // Actually this is just defensive programming. It is currently not
      // possible that lastcolumn==0 and print_last_column_on_own_line==true
      // at the same time, because lastcolumn==0 => tabstop[lastcolumn] == 0 =>
      // rightwidth==width => rightwidth>=last_column_min_width  (unless someone passes
      // a bullshit value >100 for last_column_min_percent) => the above if condition
      // is false => print_last_column_on_own_line==false
      if (lastcolumn == 0)
        print_last_column_on_own_line = false;

      LineWrapper lastColumnLineWrapper(width - rightwidth, width);
      LineWrapper interjectionLineWrapper(0, width);

      part.restartTable();

      /***************** Print out all rows of the table *************************************/

      while (part.nextRow())
      {
        int x = -1;
        while (part.next())
        {
          if (part.column() > lastcolumn)
            continue; // drop excess columns (can happen if lastcolumn == maxcolumns-1)

          if (part.column() == 0)
          {
            if (x >= 0)
              write (NATIVE_TEXT ("\n"), 1);
            x = 0;
          }

          indent(write, x, tabstop[part.column()]);

          if ((part.column() < lastcolumn)
              && (part.column() > 0 || part.line() > 0 || part.data()[part.length()] == '\t'
                  || part.data()[part.length()] == '\v'))
          {
            write(part.data(), part.length());
            x += part.screenLength();
          }
          else // either part.column() == lastcolumn or we are in the special case of
               // an interjection that doesn't contain \v or \t
          {
            // NOTE: This code block is not necessarily executed for
            // each line, because some rows may have fewer columns.

            LineWrapper& lineWrapper = (part.column() == 0) ? interjectionLineWrapper : lastColumnLineWrapper;

            if (!print_last_column_on_own_line)
              lineWrapper.process(write, part.data(), part.length());
          }
        } // while

        if (print_last_column_on_own_line)
        {
          part.restartRow();
          while (part.next())
          {
            if (part.column() == lastcolumn)
            {
              write(NATIVE_TEXT ("\n"), 1);
              int _ = 0;
              indent(write, _, width - rightwidth);
              lastColumnLineWrapper.process(write, part.data(), part.length());
            }
          }
        }

        write (NATIVE_TEXT ("\n"), 1);
        lastColumnLineWrapper.flush(write);
        interjectionLineWrapper.flush(write);
      }
    }
  }

} // namespace option

// eof optionparser.cpp
