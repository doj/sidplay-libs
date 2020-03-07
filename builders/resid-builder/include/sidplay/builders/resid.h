/***************************************************************************
                          resid.h  -  ReSid Interface
                             -------------------
    begin                : Wed Jun 21 2006
    copyright            : (C) 2006 by Simon White
    email                : s_a_white@email.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _resid_h_
#define _resid_h_

#include <sidplay/sidbuilder.h>

class IReSIDBuilder: public ISidBuilder
{
public:
    static const Iid &iid () {
        SIDIID(0x90a0aa02, 0xf272, 0x435d, 0x8f6b, 0x71b4, 0x5ac2f99f);
    }

    virtual uint create   (uint sids) = 0;
    virtual uint devices  (bool used) = 0;
    virtual void filter   (bool enable) = 0;
    virtual void filter   (const sid_filter_t *filter) = 0;
    virtual void remove   (void) = 0;
    virtual void sampling (uint_least32_t freq) = 0;
};

typedef IReSIDBuilder ReSIDBuilder;

extern "C" ISidUnknown *ReSIDBuilderCreate (const char * name);

#endif // _resid_h_
