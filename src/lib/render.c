/* vi:set ts=4 sw=4 expandtab:
 *
 * Copyright 2016, Chris Leishman (http://github.com/cleishm)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "../../config.h"
#include "render.h"
#include "util.h"
#include <assert.h>
#if HAVE_LANGINFO_CODESET
#include <langinfo.h>
#endif


struct border_glifs
{
    const char *horizontal_line;
    const char *head_line;
    const char *vertical_line;
    const char *top_corners[3];
    const char *head_corners[3];
    const char *middle_corners[3];
    const char *bottom_corners[3];
    const char *overflow;
};


static const struct border_glifs ascii_border_glifs =
    { .horizontal_line = "-",
      .head_line = "-",
      .vertical_line = "|",
      .top_corners = { "+", "+", "+" },
      .head_corners = { "+", "+", "+" },
      .middle_corners = { "+", "+", "+" },
      .bottom_corners = { "+", "+", "+" },
      .overflow = "=" };

#if HAVE_LANGINFO_CODESET
static const struct border_glifs u8_border_glifs =
    { .horizontal_line = u8"\u2500",
      .head_line = u8"\u2550",
      .vertical_line = u8"\u2502",
      .top_corners = { u8"\u250C", u8"\u252C", u8"\u2510" },
      .head_corners = { u8"\u255E", u8"\u256A", u8"\u2561" },
      .middle_corners = { u8"\u251C", u8"\u253C", u8"\u2524" },
      .bottom_corners = { u8"\u2514", u8"\u2534", u8"\u2518" },
      .overflow = u8"\u2026" };
#endif


static ssize_t render_field(FILE *stream, const char *s, size_t n,
        unsigned int width, uint_fast32_t flags);
static int write_unprintable(FILE *stream, int codepoint, int width);


uint_fast32_t normalize_render_flags(uint_fast32_t flags)
{
#if HAVE_LANGINFO_CODESET
    // check if the codeset will support extended border drawing,
    // and set NEO4J_RENDER_ASCII_ART if it will not.
    if (flags & NEO4J_RENDER_ASCII)
    {
        flags |= NEO4J_RENDER_ASCII_ART;
    }
    else
    {
        const char *codeset = nl_langinfo(CODESET);
        if (strcmp(codeset, "UTF-8") != 0)
        {
            flags |= NEO4J_RENDER_ASCII_ART;
        }
    }
#else
    flags |= NEO4J_RENDER_ASCII_ART;
#endif
    return flags;
}


static const struct border_glifs *glifs_for_encoding(uint_fast32_t flags)
{
#if HAVE_LANGINFO_CODESET
    if (!(flags & NEO4J_RENDER_ASCII_ART))
    {
        return &u8_border_glifs;
    }
#endif
    return &ascii_border_glifs;
}


int render_border_line(FILE *stream, border_line_t line_type,
        uint_fast32_t flags)
{
    const struct border_glifs *glifs = glifs_for_encoding(flags);
    const char *glif;
    switch (line_type)
    {
    case HORIZONTAL_LINE:
        glif = glifs->horizontal_line;
        break;
    case HEAD_LINE:
        glif = glifs->head_line;
        break;
    case VERTICAL_LINE:
        glif = glifs->vertical_line;
        break;
    case TOP_LEFT_CORNER:
        glif = glifs->top_corners[0];
        break;
    case TOP_MIDDLE_CORNER:
        glif = glifs->top_corners[1];
        break;
    case TOP_RIGHT_CORNER:
        glif = glifs->top_corners[2];
        break;
    case HEAD_LEFT_CORNER:
        glif = glifs->head_corners[0];
        break;
    case HEAD_MIDDLE_CORNER:
        glif = glifs->head_corners[1];
        break;
    case HEAD_RIGHT_CORNER:
        glif = glifs->head_corners[2];
        break;
    case MIDDLE_LEFT_CORNER:
        glif = glifs->middle_corners[0];
        break;
    case MIDDLE_MIDDLE_CORNER:
        glif = glifs->middle_corners[1];
        break;
    case MIDDLE_RIGHT_CORNER:
        glif = glifs->middle_corners[2];
        break;
    case BOTTOM_LEFT_CORNER:
        glif = glifs->bottom_corners[0];
        break;
    case BOTTOM_MIDDLE_CORNER:
        glif = glifs->bottom_corners[1];
        break;
    default:
        assert(line_type == BOTTOM_RIGHT_CORNER);
        glif = glifs->bottom_corners[2];
        break;
    }

    if (fputs(glif, stream) == EOF)
    {
        return -1;
    }
    return 0;
}


int render_hrule(FILE *stream, unsigned int ncolumns,
        unsigned int *widths, hline_position_t position,
        bool undersize, uint_fast32_t flags)
{
    const struct border_glifs *glifs = glifs_for_encoding(flags);
    const char * const *corners;
    const char *line;
    switch (position)
    {
    case HLINE_TOP:
        corners = glifs->top_corners;
        line = glifs->horizontal_line;
        break;
    case HLINE_HEAD:
        corners = glifs->head_corners;
        line = glifs->head_line;
        break;
    case HLINE_BOTTOM:
        corners = glifs->bottom_corners;
        line = glifs->horizontal_line;
        break;
    default:
        assert(position == HLINE_MIDDLE);
        corners = glifs->middle_corners;
        line = glifs->horizontal_line;
        break;
    }
    for (unsigned int i = 0, corner = 0; i < ncolumns; ++i)
    {
        if (widths[i] == 0)
        {
            continue;
        }
        if (fputs(corners[corner], stream) == EOF)
        {
            return -1;
        }
        corner = 1;
        for (unsigned int w = widths[i]; w > 0; --w)
        {
            if (fputs(line, stream) == EOF)
            {
                return -1;
            }
        }
    }
    if (fputs(corners[undersize? 1 : 2], stream) == EOF)
    {
        return -1;
    }
    if (undersize && fputs(line, stream) == EOF)
    {
        return -1;
    }
    if (fputc('\n', stream) == EOF)
    {
        return -1;
    }
    return 0;
}


int render_row(FILE *stream, unsigned int ncolumns,
        const unsigned int *widths, bool undersize, uint_fast32_t flags,
        render_row_callback_t callback, void *cdata)
{
    struct fields
    {
        const char *s;
        size_t n;
        char *dup;
    };

    struct fields *fields = NULL;
    if ((flags & NEO4J_RENDER_WRAP_VALUES) &&
            (fields = calloc(ncolumns, sizeof(struct fields))) == NULL)
    {
        return -1;
    }
    bool wrap = false;

    int err = -1;

    const struct border_glifs *glifs = glifs_for_encoding(flags);
    for (unsigned int i = 0; i < ncolumns; ++i)
    {
        if (widths[i] == 0)
        {
            continue;
        }
        if (fputs(glifs->vertical_line, stream) == EOF ||
                fputc(' ', stream) == EOF)
        {
            goto cleanup;
        }

        assert(widths[i] >= 2);
        unsigned int value_width = widths[i] - 2;
        ssize_t n = 0;
        const char *s = NULL;
        bool duplicate = false;
        if (callback != NULL && (n = callback(cdata, i, &s, &duplicate)) < 0)
        {
            goto cleanup;
        }
        assert(n == 0 || s != NULL);

        ssize_t consumed = render_field(stream, s, n, value_width, flags);
        if (consumed < 0)
        {
            goto cleanup;
        }

        if (consumed >= n)
        {
            if (fputc(' ', stream) == EOF)
            {
                goto cleanup;
            }
        }
        else
        {
            if (fputs(glifs->overflow, stream) == EOF)
            {
                goto cleanup;
            }

            if (flags & NEO4J_RENDER_WRAP_VALUES)
            {
                s += consumed;
                n -= consumed;
                if (duplicate)
                {
                    if ((fields[i].dup = memdup(s, n)) == NULL)
                    {
                        goto cleanup;
                    }
                    s = fields[i].dup;
                }
                fields[i].s = s;
                fields[i].n = n;
                wrap = true;
            }
        }
    }

    if (fputs(glifs->vertical_line, stream) == EOF)
    {
        goto cleanup;
    }
    if (undersize && fputs(glifs->overflow, stream) == EOF)
    {
        goto cleanup;
    }
    if (fputc('\n', stream) == EOF)
    {
        goto cleanup;
    }

    while (wrap)
    {
        wrap = false;

        for (unsigned int i = 0; i < ncolumns; ++i)
        {
            if (widths[i] == 0)
            {
                continue;
            }
            assert(widths[i] >= 2);
            unsigned int value_width = widths[i] - 2;
            size_t n = fields[i].n;
            const char *s = fields[i].s;

            if (fputs(glifs->vertical_line, stream) == EOF ||
                    fputs((n > 0)? glifs->overflow : " ", stream) == EOF)
            {
                goto cleanup;
            }

            ssize_t consumed = render_field(stream, s, n, value_width, flags);
            if (consumed < 0)
            {
                goto cleanup;
            }

            if ((size_t)consumed >= n)
            {
                if (fputc(' ', stream) == EOF)
                {
                    goto cleanup;
                }
            }
            else
            {
                if (fputs(glifs->overflow, stream) == EOF)
                {
                    goto cleanup;
                }

                fields[i].s = s + consumed;
                fields[i].n = n - consumed;
                wrap = true;
            }
        }

        if (fputs(glifs->vertical_line, stream) == EOF)
        {
            goto cleanup;
        }
        if (undersize && fputs(glifs->overflow, stream) == EOF)
        {
            goto cleanup;
        }
        if (fputc('\n', stream) == EOF)
        {
            goto cleanup;
        }
    }

    err = 0;

    int errsv;
cleanup:
    errsv = errno;
    if (fields != NULL)
    {
        for (unsigned int i = ncolumns; (i--) > 0;)
        {
            free(fields[i].dup);
        }
        free(fields);
    }
    errno = errsv;
    return err;
}


ssize_t render_field(FILE *stream, const char *s, size_t n, unsigned int width,
        uint_fast32_t flags)
{
    unsigned int used = 0;
    const char *e = s + n;

    while (used < width && s < e)
    {
        size_t bytes = SIZE_MAX;
        int cp = neo4j_u8codepoint(s, &bytes);
        if (cp < 0)
        {
            return -1;
        }
        assert(bytes > 0);
        int cpwidth;
        if ((bytes > 1 && (flags & NEO4J_RENDER_ASCII)) ||
                (cpwidth = neo4j_u8cpwidth(cp)) < 0)
        {
            cpwidth = write_unprintable(stream, cp, width);
            if (cpwidth < 0)
            {
                return -1;
            }
        }
        else
        {
            if ((used + cpwidth) > width)
            {
                break;
            }
            if (fwrite(s, sizeof(char), bytes, stream) < bytes)
            {
                return -1;
            }
        }
        s += bytes;
        used += cpwidth;
    }

    for (; used < width; ++used)
    {
        if (fputc(' ', stream) == EOF)
        {
            return -1;
        }
    }
    return n - (e - s);
}


int write_unprintable(FILE *stream, int codepoint, int width)
{
    assert(codepoint >= 0);
    char buf[10];
    char *replacement;
    unsigned int n = 2;
    switch (codepoint)
    {
    case '\a':
        replacement = "\\a";
        break;
    case '\b':
        replacement = "\\b";
        break;
    case '\f':
        replacement = "\\f";
        break;
    case '\n':
        replacement = "\\n";
        break;
    case '\r':
        replacement = "\\r";
        break;
    case '\t':
        replacement = "\\t";
        break;
    case '\v':
        replacement = "\\v";
        break;
    default:
        replacement = buf;
        if (codepoint <= 0xFFFF)
        {
            if (snprintf(buf, sizeof(buf), "\\u%04X", codepoint) < 0)
            {
                return -1;
            }
            n = 6;
        }
        else
        {
            if (snprintf(buf, sizeof(buf), "\\U%08X", codepoint) < 0)
            {
                return -1;
            }
            n = 10;
        }
        break;
    }
    unsigned int towrite = min(n, width);
    if (fwrite(replacement, sizeof(char), towrite, stream) < towrite)
    {
        return -1;
    }
    return n;
}