/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2009 Red Hat, Inc.
 * Copyright (C) 2009 Shaun McCance <shaunm@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Shaun McCance <shaunm@gnome.org>
 */

#include "config.h"

#include <glib/gi18n.h>

#include "yelp-bz2-decompressor.h"
#include "yelp-lzma-decompressor.h"
#include "yelp-magic-decompressor.h"

static void yelp_magic_decompressor_iface_init          (GConverterIface *iface);

struct _YelpMagicDecompressor
{
    GObject parent_instance;

    GConverter *magic_decoder_ring;

    gboolean first;
};

G_DEFINE_TYPE_WITH_CODE (YelpMagicDecompressor, yelp_magic_decompressor, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_CONVERTER,
                                                yelp_magic_decompressor_iface_init))

static void
yelp_magic_decompressor_dispose (GObject *object)
{
    YelpMagicDecompressor *decompressor;

    g_object_unref (decompressor->magic_decoder_ring);

    G_OBJECT_CLASS (yelp_magic_decompressor_parent_class)->dispose (object);
}

static void
yelp_magic_decompressor_init (YelpMagicDecompressor *decompressor)
{
    decompressor->magic_decoder_ring = NULL;
    decompressor->first = TRUE;
}

static void
yelp_magic_decompressor_class_init (YelpMagicDecompressorClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->dispose = yelp_magic_decompressor_dispose;
}

YelpMagicDecompressor *
yelp_magic_decompressor_new (void)
{
    YelpMagicDecompressor *decompressor;

    decompressor = g_object_new (YELP_TYPE_MAGIC_DECOMPRESSOR, NULL);

    return decompressor;
}

static void
yelp_magic_decompressor_reset (GConverter *converter)
{
    YelpMagicDecompressor *decompressor = YELP_MAGIC_DECOMPRESSOR (converter);

    if (decompressor->magic_decoder_ring)
        g_converter_reset (decompressor->magic_decoder_ring);
}

static GConverterResult
yelp_magic_decompressor_convert (GConverter *converter,
                                 const void *inbuf,
                                 gsize       inbuf_size,
                                 void       *outbuf,
                                 gsize       outbuf_size,
                                 GConverterFlags flags,
                                 gsize      *bytes_read,
                                 gsize      *bytes_written,
                                 GError    **error)
{
    YelpMagicDecompressor *decompressor;

    decompressor = YELP_MAGIC_DECOMPRESSOR (converter);

    if (decompressor->first) {
        decompressor->first = FALSE;
        /* If input_size is less than two the first time, we end up
         * not getting detection.  Might be worth addressing.  Not
         * sure I care.
         *
         * The two-byte magic we're doing here is not sufficient in
         * the general case.  It is sufficient for the specific data
         * Yelp deals with.
         */
        if (inbuf_size <= 2)
            ;
        else if (((gchar *) inbuf)[0] == 'B' &&
                 ((gchar *) inbuf)[1] == 'Z') {
            decompressor->magic_decoder_ring = (GConverter *) yelp_bz2_decompressor_new ();
        }
        else if (((gchar *) inbuf)[0] == ']' &&
                 ((gchar *) inbuf)[1] == '\0') {
            decompressor->magic_decoder_ring = (GConverter *) yelp_lzma_decompressor_new ();
        }
        else {
            decompressor->magic_decoder_ring =
                (GConverter *) g_zlib_decompressor_new (G_ZLIB_COMPRESSOR_FORMAT_GZIP);
        }
    }

    return g_converter_convert (decompressor->magic_decoder_ring,
                                inbuf, inbuf_size,
                                outbuf, outbuf_size,
                                flags,
                                bytes_read, bytes_written,
                                error);

    g_assert_not_reached ();
}

static void
yelp_magic_decompressor_iface_init (GConverterIface *iface)
{
  iface->convert = yelp_magic_decompressor_convert;
  iface->reset = yelp_magic_decompressor_reset;
}