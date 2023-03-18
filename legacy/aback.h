/*****************************************************************************
  The MIT License

  Copyright © 2010 Pavel Karelin (hkarel), <hkarel@yandex.ru>

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*****************************************************************************/

#pragma once

//#include "simple_ptr.h"

/**
  AutoBackMember - сервисная структура, используется для присвоения
  переменной-члену класса временного значения в пределах кодового
  блока. При выходе из кодового блока переменной будет возвращено
  ее первоначальное значение.
  Реализован механизм отмены автовозврата предыдущего параметра.
*/
struct AutoBackMemberBase
{
    virtual ~AutoBackMemberBase() {}
};


template<
    typename ClassT,
    typename MemberT
>
class AutoBackMember : public AutoBackMemberBase
{
    ClassT* c;
    MemberT m;
    MemberT (ClassT::*pm);
    bool* cancel; // Используется для отмены автовозврата.

	// Без реализации
    AutoBackMember();
    AutoBackMember& operator== (const AutoBackMember&);

    AutoBackMember(/*const*/ ClassT* c, MemberT (ClassT::*pm), MemberT m, bool* cancel) {
        this->c = /*(ClassT*)*/c;
        this->pm = pm;
        this->m = (c->*pm);
        this->cancel = cancel;
        (c->*pm) = m;
    }

public:
    // Без реализации (Фиктивный конструктор (?? почему то VS требует его как public)
    AutoBackMember(const AutoBackMember& abm);
    ~AutoBackMember() {if (cancel && *cancel) c = 0; if (c && pm) (c->*pm) = m;}

    template<typename T, typename M>
    friend inline AutoBackMember<T, M> createAutoBackMember(/*const*/ T*, M (T::*), M, bool* cancel);
};

template<typename T, typename M>
inline AutoBackMember<T, M> createAutoBackMember(/*const*/ T* c,
                                                 M (T::*pm), M m, bool* cancel = 0) {
    return AutoBackMember<T, M>(c, pm, m, cancel);
}


template<
    typename ClassT,
    typename MemberT
>
class AutoBackMemberVlt : public AutoBackMemberBase
{
    ClassT* c;
    MemberT m;
    volatile MemberT (ClassT::*pm);
    bool* cancel; // Используется для отмены автовозврата.

	// Без реализации
    AutoBackMemberVlt();
    AutoBackMemberVlt& operator== (const AutoBackMemberVlt&);

    AutoBackMemberVlt(/*const*/ ClassT* c,
                      volatile MemberT (ClassT::*pm), MemberT m, bool* cancel) {
        this->c = /*(ClassT*)*/c;
        this->pm = pm;
        this->m = (c->*pm);
        this->cancel = cancel;
        (c->*pm) = m;
    }

public:
    // Без реализации (Фиктивный конструктор (?? почему то VS требует его как public)
    AutoBackMemberVlt(const AutoBackMemberVlt&);
    ~AutoBackMemberVlt() {if (cancel && *cancel) c = 0; if (c && pm) (c->*pm) = m;}

    template<typename T, typename M>
    friend inline AutoBackMemberVlt<T, M> createAutoBackMember(/*const*/ T*, volatile M (T::*), M, bool* cancel);
};

template<typename T, typename M>
inline AutoBackMemberVlt<T, M> createAutoBackMember(/*const*/ T* c,
                                                    volatile M (T::*pm), M m, bool* cancel = 0) {
    return AutoBackMemberVlt<T, M>(c, pm, m, cancel);
}


template<
    typename ClassT,
    typename ReturnT,
    typename MemberT
>
class AutoBackMemberFunc : public AutoBackMemberBase
{
    ClassT* c;
    MemberT p;
    ReturnT (ClassT::*pf)(MemberT);
    bool* cancel; // Используется для отмены автовозврата.

    // Без реализации
    AutoBackMemberFunc();
    AutoBackMemberFunc& operator== (const AutoBackMemberFunc&);

    //AutoBackMemberFunc(AutoBackMemberFunc&& abm)  {
    //    c = abm.c;
    //    pf = abm.pf;
    //    p = abm.p;
    //    cancel = abm.cancel;
    //    abm.c = 0;
    //}

    AutoBackMemberFunc(/*const*/ ClassT* c,
                       ReturnT (ClassT::*pf)(MemberT), MemberT m, MemberT p, bool* cancel) {
        this->c = /*(ClassT*)*/c;
        this->pf = pf;
        this->p = p;
        this->cancel = cancel;
        (c->*pf)(m);
    }

public:
    // Без реализации (Фиктивный конструктор (?? почему то VS требует его как public)
    AutoBackMemberFunc(const AutoBackMemberFunc&);
    ~AutoBackMemberFunc() {if (cancel && *cancel) c = 0; if (c && pf) (c->*pf)(p);}

    template<typename T, typename R, typename M>
    friend inline AutoBackMemberFunc<T, R, M> createAutoBackMemberFunc(/*const*/ T*, R (T::*)(M), M, M, bool*);
};

template<typename T, typename R, typename M>
inline AutoBackMemberFunc<T, R, M> createAutoBackMemberFunc(/*const*/ T* c,
                                                            R (T::*pf)(M),
                                                            M m, M p, bool* cancel = 0) {
    return AutoBackMemberFunc<T, R, M>(c, pf, m, p, cancel);
}


#define ABACK(MEMBER_, VALUE_) \
    const AutoBackMemberBase& auto_back_member__(createAutoBackMember(this, MEMBER_, VALUE_));  \
    (void) auto_back_member__;

#define ABACK2(MEMBER_, VALUE_) \
    const AutoBackMemberBase& auto_back_member_2_(createAutoBackMember(this, MEMBER_, VALUE_)); \
    (void) auto_back_member_2_;

#define ABACK3(MEMBER_, VALUE_) \
    const AutoBackMemberBase& auto_back_member_3_(createAutoBackMember(this, MEMBER_, VALUE_)); \
    (void) auto_back_member_3_;

#define ABACK4(MEMBER_, VALUE_) \
    const AutoBackMemberBase& auto_back_member_4_(createAutoBackMember(this, MEMBER_, VALUE_)); \
    (void) auto_back_member_4_;

#define ABACK_C(MEMBER_, VALUE_, CANCEL_) \
    const AutoBackMemberBase& auto_back_member_c_(createAutoBackMember(this, MEMBER_, VALUE_, CANCEL_));  \
    (void) auto_back_member_c_;

#define ABACK2_C(MEMBER_, VALUE_, CANCEL_) \
    const AutoBackMemberBase& auto_back_member_2c_(createAutoBackMember(this, MEMBER_, VALUE_, CANCEL_)); \
    (void) auto_back_member_2c_;

#define ABACK3_C(MEMBER_, VALUE_, CANCEL_) \
    const AutoBackMemberBase& auto_back_member_3c_(createAutoBackMember(this, MEMBER_, VALUE_, CANCEL_)); \
    (void) auto_back_member_3c_;

#define ABACK4_C(MEMBER_, VALUE_, CANCEL_) \
    const AutoBackMemberBase& auto_back_member_4c_(createAutoBackMember(this, MEMBER_, VALUE_, CANCEL_)); \
    (void) auto_back_member_4c_;


#define ABACK_F(MEMBER_, VALUE_, PREVAL_) \
    const AutoBackMemberBase& auto_back_member_func__(createAutoBackMemberFunc(this, MEMBER_, VALUE_, PREVAL_)); \
    (void) auto_back_member_func__;

#define ABACK2_F(MEMBER_, VALUE_, PREVAL_) \
    const AutoBackMemberBase& auto_back_member_func_2_(createAutoBackMemberFunc(this, MEMBER_, VALUE_, PREVAL_)); \
    (void) auto_back_member_func_2_;

#define ABACK3_F(MEMBER_, VALUE_, PREVAL_) \
    const AutoBackMemberBase& auto_back_member_func_3_(createAutoBackMemberFunc(this, MEMBER_, VALUE_, PREVAL_)); \
    (void) auto_back_member_func_3_;

#define ABACK4_F(MEMBER_, VALUE_, PREVAL_) \
    const AutoBackMemberBase& auto_back_member_func_4_(createAutoBackMemberFunc(this, MEMBER_, VALUE_, PREVAL_)); \
    (void) auto_back_member_func_4_;

#define ABACK_FC(MEMBER_, VALUE_, PREVAL_, CANCEL_) \
    const AutoBackMemberBase& auto_back_member_func_c_(createAutoBackMemberFunc(this, MEMBER_, VALUE_, PREVAL_, CANCEL_)); \
    (void) auto_back_member_func_c_;

#define ABACK2_FC(MEMBER_, VALUE_, PREVAL_, CANCEL_) \
    const AutoBackMemberBase& auto_back_member_func_2c_(createAutoBackMemberFunc(this, MEMBER_, VALUE_, PREVAL_, CANCEL_)); \
    (void) auto_back_member_func_2c_;

#define ABACK3_FC(MEMBER_, VALUE_, PREVAL_, CANCEL_) \
    const AutoBackMemberBase& auto_back_member_func_3c_(createAutoBackMemberFunc(this, MEMBER_, VALUE_, PREVAL_, CANCEL_)); \
    (void) auto_back_member_func_3c_;

#define ABACK4_FC(MEMBER_, VALUE_, PREVAL_, CANCEL_) \
    const AutoBackMemberBase& auto_back_member_func_4c_(createAutoBackMemberFunc(this, MEMBER_, VALUE_, PREVAL_, CANCEL_)); \
    (void) auto_back_member_func_4c_;

