
#include <qdatetime.h>
#include <qstring.h>
#include <qregexp.h>
#include <qapplication.h>
#include <qmetaobject.h>
#include <qtoolbar.h>
#include <qtabbar.h>
#include <qdir.h>
#include <qvariant.h>
#include <qhash.h>

#include "smoke.h"
#include "field.h"
#include "table.h"

#undef DEBUG
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef __USE_POSIX
#define __USE_POSIX
#endif
#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#if PERL_VERSION == 6 && PERL_SUBVERSION == 0
  #include <qtextcodec.h>
#endif

#include "marshall.h"
#include "perlqt.h"
#include "smokeperl.h"

#ifndef HINT_BYTES
#define HINT_BYTES HINT_BYTE
#endif

#ifndef PERL_MAGIC_tiedscalar
#define PERL_MAGIC_tiedscalar 'q'
#endif

extern HV* pointer_map;
static QHash<int,Smoke::Index*> *dtorcache= 0;
static QHash<int,Smoke::Index*> *cctorcache= 0;

int smokeperl_free(pTHX_ SV *sv, MAGIC *mg) {
	smokeperl_object *o = (smokeperl_object*)mg->mg_ptr;
	
	const char *className = o->smoke->classes[o->classId].className;
	if(o->allocated && o->ptr) {
		if(do_debug && (do_debug & qtdb_gc)) fprintf(stderr, "Deleting (%s*)%p\n", className, o->ptr);
		SmokeClass sc(o->smoke, o->classId);
		if(sc.hasVirtual()) 
			unmapPointer(o, o->classId, 0);
		Smoke::Index *pmeth = dtorcache->value( o->classId );
		if(pmeth) {
			Smoke::Method &m = o->smoke->methods[o->smoke->methodMaps[*pmeth].method];
			Smoke::ClassFn fn = o->smoke->classes[m.classId].classFn;
			Smoke::StackItem i[1];
			(*fn)(m.method, o->ptr, i);
		} else {
			char *methodName = new char[strlen(className) + 2];
			methodName[0] = '~';
			strcpy(methodName + 1, className);
			Smoke::Index nameId = o->smoke->idMethodName(methodName);
			Smoke::Index meth = o->smoke->findMethod(o->classId, nameId);
			if(meth > 0) {
				dtorcache->insert(o->classId, new Smoke::Index(meth));
				Smoke::Method &m = o->smoke->methods[o->smoke->methodMaps[meth].method];
				Smoke::ClassFn fn = o->smoke->classes[m.classId].classFn;
				Smoke::StackItem i[1];
				(*fn)(m.method, o->ptr, i);
			}
			delete[] methodName;
		}
    }
    return 0;
}

struct mgvtbl vtbl_smoke = { 0, 0, 0, 0, smokeperl_free };

bool matches_arg(Smoke *smoke, Smoke::Index meth, Smoke::Index argidx, const char *argtype) {
	Smoke::Index *arg = smoke->argumentList + smoke->methods[meth].args + argidx;
	SmokeType type = SmokeType(smoke, *arg);
	if(type.name() && !strcmp(type.name(), argtype))
		return true;
	return false;
}

void *construct_copy(smokeperl_object *o) {
	Smoke::Index *pccMeth = cctorcache->value(o->classId);
	Smoke::Index ccMeth = 0;
    if(!pccMeth) {
		const char *className = o->smoke->className(o->classId);
		int classNameLen = strlen(className);
		char *ccSig = new char[classNameLen + 2];       // copy constructor signature
		strcpy(ccSig, className);
		strcat(ccSig, "#");
		Smoke::Index ccId = o->smoke->idMethodName(ccSig);
		delete[] ccSig;

		char *ccArg = new char[classNameLen + 8];
		sprintf(ccArg, "const %s&", className);
		ccMeth = o->smoke->findMethod(o->classId, ccId);

		if(!ccMeth) {
			cctorcache->insert(o->classId, new Smoke::Index(0));
                        delete [] ccArg;
			return 0;
		}
		Smoke::Index method =  o->smoke->methodMaps[ccMeth].method;
		if(method > 0) {
			// Make sure it's a copy constructor
			if(!matches_arg(o->smoke, method, 0, ccArg)) {
				delete[] ccArg;
				cctorcache->insert(o->classId, new Smoke::Index(0));
				return 0;
			}
			ccMeth = method;
		} else {
			// ambiguous method, pick the copy constructor
			Smoke::Index i = -method;
			while(o->smoke->ambiguousMethodList[i]) {
				if(matches_arg(o->smoke, o->smoke->ambiguousMethodList[i], 0, ccArg))
				break;
					i++;
			}
			ccMeth = o->smoke->ambiguousMethodList[i];
			if(!ccMeth) {
				cctorcache->insert(o->classId, new Smoke::Index(0));
                                delete[] ccArg;
				return 0;
			}
		}
                delete [] ccArg;
		cctorcache->insert(o->classId, new Smoke::Index(ccMeth));
	} else {
		ccMeth = *pccMeth;
		if(!ccMeth)
			return 0;
	}
	// Okay, ccMeth is the copy constructor. Time to call it.
	Smoke::StackItem args[2];
	args[0].s_voidp = 0;
	args[1].s_voidp = o->ptr;
	Smoke::ClassFn fn = o->smoke->classes[o->classId].classFn;
	(*fn)(o->smoke->methods[ccMeth].method, 0, args);
	return args[0].s_voidp;
}

static void marshall_basetype(Marshall *m) {
	switch(m->type().elem()) {
		case Smoke::t_bool:
		switch(m->action()) {
			case Marshall::FromSV:
				m->item().s_bool = SvTRUE(m->var()) ? true : false;
				break;
			case Marshall::ToSV:
				sv_setsv_mg(m->var(), boolSV(m->item().s_bool));
				break;
			default:
				m->unsupported();
				break;
		}
		break;
		case Smoke::t_char:
		switch(m->action()) {
			case Marshall::FromSV:
				m->item().s_char = (char)SvIV(m->var());
				break;
			case Marshall::ToSV:
				sv_setiv_mg(m->var(), (IV)m->item().s_char);
				break;
			default:
				m->unsupported();
				break;
		}
		break;
		case Smoke::t_uchar:
		switch(m->action()) {
			case Marshall::FromSV:
				m->item().s_uchar = (unsigned char)SvIV(m->var());
				break;
			case Marshall::ToSV:
				sv_setiv_mg(m->var(), (IV)m->item().s_uchar);
				break;
			default:
				m->unsupported();
				break;
		}
		break;
		case Smoke::t_short:
		switch(m->action()) {
			case Marshall::FromSV:
				m->item().s_short = (short)SvIV(m->var());
				break;
			case Marshall::ToSV:
				sv_setiv_mg(m->var(), (IV)m->item().s_short);
				break;
			default:
				m->unsupported();
				break;
		}
		break;
		case Smoke::t_ushort:
		switch(m->action()) {
			case Marshall::FromSV:
				m->item().s_ushort = (unsigned short)SvIV(m->var());
				break;
			case Marshall::ToSV:
				sv_setiv_mg(m->var(), (IV)m->item().s_ushort);
				break;
			default:
				m->unsupported();
				break;
		}
		break;
		case Smoke::t_int:
		switch(m->action()) {
			case Marshall::FromSV:
				m->item().s_int = (int)SvIV(m->var());
				break;
			case Marshall::ToSV:
				sv_setiv_mg(m->var(), (IV)m->item().s_int);
				break;
			default:
				m->unsupported();
				break;
		}
		break;
		case Smoke::t_uint:
		switch(m->action()) {
			case Marshall::FromSV:
				m->item().s_uint = (unsigned int)SvIV(m->var());
				break;
			case Marshall::ToSV:
				sv_setiv_mg(m->var(), (IV)m->item().s_uint);
				break;
			default:
				m->unsupported();
				break;
		}
		break;
		case Smoke::t_long:
		switch(m->action()) {
			case Marshall::FromSV:
				m->item().s_long = (long)SvIV(m->var());
				break;
			case Marshall::ToSV:
				sv_setiv_mg(m->var(), (IV)m->item().s_long);
				break;
			default:
				m->unsupported();
				break;
		}
		break;
		case Smoke::t_ulong:
		switch(m->action()) {
			case Marshall::FromSV:
				m->item().s_ulong = (unsigned long)SvIV(m->var());
				break;
			case Marshall::ToSV:
				sv_setiv_mg(m->var(), (IV)m->item().s_ulong);
				break;
			default:
				m->unsupported();
				break;
		}
		break;
		case Smoke::t_float:
			switch(m->action()) {
			case Marshall::FromSV:
				m->item().s_float = (float)SvNV(m->var());
				break;
			case Marshall::ToSV:
				sv_setnv_mg(m->var(), (NV)m->item().s_float);
				break;
			default:
				m->unsupported();
				break;
		}
		break;
		case Smoke::t_double:
		switch(m->action()) {
			case Marshall::FromSV:
				m->item().s_double = (double)SvNV(m->var());
				break;
			case Marshall::ToSV:
				sv_setnv_mg(m->var(), (NV)m->item().s_double);
				break;
			default:
				m->unsupported();
				break;
		}
		break;
		case Smoke::t_enum:
		switch(m->action()) {
			case Marshall::FromSV:
				m->item().s_enum = (long)SvIV(m->var());
				break;
			case Marshall::ToSV:
				sv_setiv_mg(m->var(), (IV)m->item().s_enum);
				break;
			default:
				m->unsupported();
				break;
		}
		break;
		case Smoke::t_class:
		switch(m->action()) {
			case Marshall::FromSV:
			{
				smokeperl_object *o = sv_obj_info(m->var());
				if(!o || !o->ptr) {
					if(m->type().isRef()) {
						warn("References can't be null or undef\n");
						m->unsupported();
					}
					m->item().s_class = 0;
					break;
				}
				void *ptr = o->ptr;
				if(!m->cleanup() && m->type().isStack()) {
					void *p = construct_copy(o);
					if(p)
						ptr = p;
				}
				const Smoke::Class &c = m->smoke()->classes[m->type().classId()];
				ptr = o->smoke->cast(
					ptr,				// pointer
					o->classId,				// from
					o->smoke->idClass(c.className)	// to
				);
				m->item().s_class = ptr;
				break;
			}
			break;
			case Marshall::ToSV:
			{
				if(!m->item().s_voidp) {
					sv_setsv_mg(m->var(), &PL_sv_undef);
					break;
				}
				void *p = m->item().s_voidp;
				SV *obj = getPointerObject(p);
				if(obj) {
					sv_setsv_mg(m->var(), obj);
					break;
				}
				HV *hv = newHV();
				obj = newRV_noinc((SV*)hv);
				// TODO: Generic mapping from C++ classname to Qt classname
		
				smokeperl_object o;
				o.smoke = m->smoke();
				o.classId = m->type().classId();
				o.ptr = p;
				o.allocated = false;
		
				if(m->type().isStack())
					o.allocated = true;
		
				char *buf = m->smoke()->binding->className(m->type().classId());
				sv_bless(obj, gv_stashpv(buf, TRUE));
				delete[] buf;
				if(m->type().isConst() && m->type().isRef()) {
					p = construct_copy( &o );
					if(p) {
						o.ptr = p;
						o.allocated = true;
					}
				}
				sv_magic((SV*)hv, sv_qapp, '~', (char*)&o, sizeof(o));
				MAGIC *mg = mg_find((SV*)hv, '~');
				mg->mg_virtual = &vtbl_smoke;
				sv_setsv_mg(m->var(), obj);
				SmokeClass sc( m->type() );
				if( sc.hasVirtual() )
					mapPointer(obj, &o, pointer_map, o.classId, 0);
				SvREFCNT_dec(obj);
			}
			break;
		default:
			m->unsupported();
			break;
	}
	break;
	default:
		m->unsupported();
	break;
    }
}

static void marshall_void(Marshall *) {}
static void marshall_unknown(Marshall *m) {
	m->unsupported();
}

static void marshall_charP(Marshall *m) {
	switch(m->action()) {
		case Marshall::FromSV:
		{
			SV *sv = m->var();
			if(!SvOK(sv)) {
				m->item().s_voidp = 0;
				break;
			}
			if(m->cleanup())
				m->item().s_voidp = SvPV_nolen(sv);
			else {
				STRLEN len;
				char *svstr = SvPV(sv, len);
				char *str = new char [len + 1];
				strncpy(str, svstr, len);
				str[len] = 0;
				m->item().s_voidp = str;
			}
		}
		break;
		case Marshall::ToSV:
		{
			char *p = (char*)m->item().s_voidp;
			if(p)
			sv_setpv_mg(m->var(), p);
			else
			sv_setsv_mg(m->var(), &PL_sv_undef);
			if(m->cleanup())
			delete[] p;
		}
		break;
		default:
			m->unsupported();
		break;
	}
}

void marshall_ucharP(Marshall *m) {
	switch(m->action()) {
		case Marshall::FromSV:
		{
			SV* sv = m->var();
			QByteArray *s = 0;
			MAGIC* mg = 0;
			bool hasMagic = false;
			if(SvOK(sv)) {
				if( SvTYPE(sv) == SVt_PVMG  && (mg = mg_find(sv, PERL_MAGIC_tiedscalar))
				&& sv_derived_from(mg->mg_obj, "Qt::_internal::QByteArray") ) {
					s = (QByteArray*)SvIV((SV*)SvRV(mg->mg_obj));
					hasMagic = true;
				} else {
					STRLEN len;
					char* tmp = SvPV(sv, len);
					s = new QByteArray(int(len),0);
					Copy((void*)tmp, (void*)s->data(), len, char);
					if( !m->type().isConst() && !SvREADONLY(sv) ) {
								SV* rv = newSV(0);
								sv_setref_pv(rv, "Qt::_internal::QByteArray", (void*)s);
								sv_magic(sv, rv, PERL_MAGIC_tiedscalar, Nullch, 0);
								hasMagic = true;
					}
				}
			} else {
				if( !m->type().isConst() ) {
					if(SvREADONLY(sv) && m->type().isPtr()) {
						m->item().s_voidp = 0;
						break;
					}
					s = new QByteArray(0);
					if( !SvREADONLY(sv) ) {
						SV* rv = newSV(0);
						sv_setpv_mg(sv, "");
						sv_setref_pv(rv, "Qt::_internal::QByteArray", s);
						sv_magic(sv, rv, PERL_MAGIC_tiedscalar, Nullch, 0);
						hasMagic = true;
					}
				} else
					s = new QByteArray(0);
			}
			m->item().s_voidp = s->data();
			m->next();
			if(s && !hasMagic && m->cleanup())
				delete s;
		}
		break;
		default:
			m->unsupported();
		break;
	}
}

static void marshall_QString(Marshall *m) {
	switch(m->action()) {
		case Marshall::FromSV:
		{
			SV* sv = m->var();
			QString *s = 0;
			MAGIC* mg = 0;
			bool hasMagic = false;
			if(SvOK(sv) || m->type().isStack()) {
				if( SvTYPE(sv) == SVt_PVMG  && (mg = mg_find(sv, PERL_MAGIC_tiedscalar))
				&& sv_derived_from(mg->mg_obj, "Qt::_internal::QString") ) {
					s = (QString*)SvIV((SV*)SvRV(mg->mg_obj));
					hasMagic = true;
				} else {
					COP *cop = cxstack[cxstack_ix].blk_oldcop;
					if(SvUTF8(sv))
						s = new QString(QString::fromUtf8(SvPV_nolen(sv)));
					else if(cop->op_private & HINT_LOCALE)
						s = new QString(QString::fromLocal8Bit(SvPV_nolen(sv)));
					else
						s = new QString(QString::fromLatin1(SvPV_nolen(sv)));
					if( !m->type().isConst() && !m->type().isStack() && !SvREADONLY(sv)) {
						SV* rv = newSV(0);
						sv_setref_pv(rv, "Qt::_internal::QString", (void*)s);
						sv_magic(sv, rv, PERL_MAGIC_tiedscalar, Nullch, 0);
						hasMagic = true;
					}
				}
			} else {
				if(!m->type().isConst()) {
					if(SvREADONLY(sv) && m->type().isPtr()) {
						m->item().s_voidp = 0;
						break;
					}
					s = new QString;
					if( !SvREADONLY(sv) ) {
						SV* rv = newSV(0);
						sv_setpv_mg(sv, "");
						sv_setref_pv(rv, "Qt::_internal::QString", s);
						sv_magic(sv, rv, PERL_MAGIC_tiedscalar, Nullch, 0);
						hasMagic = true;
					}
				} else
					s = new QString;
			}
			m->item().s_voidp = s;
			m->next();
			if(s && !hasMagic && m->cleanup())
				delete s;
		}
		break;
	case Marshall::ToSV:
	{
		QString *s = (QString*)m->item().s_voidp;
		if(s) {
			COP *cop = cxstack[cxstack_ix].blk_oldcop;
			if(!(cop->op_private & HINT_BYTES))
			{
				sv_setpv_mg(m->var(), (const char *)s->toUtf8());
				SvUTF8_on(m->var());
			}
			else if(cop->op_private & HINT_LOCALE)
				sv_setpv_mg(m->var(), (const char *)s->toLocal8Bit());
			else
				sv_setpv_mg(m->var(), (const char *)s->toLatin1());
		}
	    else
			sv_setsv_mg(m->var(), &PL_sv_undef);
		if(m->cleanup())
			delete s;
	}
	break;
	default:
		m->unsupported();
		break;
    }
}

static void marshall_QByteArray(Marshall *m) {
	switch(m->action()) {
		case Marshall::FromSV:
		{
			SV* sv = m->var();
			QByteArray *s = 0;
			MAGIC* mg = 0;
			bool hasMagic = false;
			if(SvOK(sv) || m->type().isStack()) {
				if( SvTYPE(sv) == SVt_PVMG  && (mg = mg_find(sv, PERL_MAGIC_tiedscalar))
				&& sv_derived_from(mg->mg_obj, "Qt::_internal::QByteArray") ) {
					s = (QByteArray*)SvIV((SV*)SvRV(mg->mg_obj));
					hasMagic = true;
				} else {
					STRLEN len;
					char* tmp = SvPV(sv, len); 
					s = new QByteArray(int(len),0);
					Copy((void*)tmp, (void*)s->data(), len, char);
					if( !m->type().isConst() && !SvREADONLY(sv) ) { // we tie also stack because of the funny QDataStream behaviour
						// fprintf(stderr, "Tying\n");
						SV* rv = newSV(0);
						sv_setref_pv(rv, "Qt::_internal::QByteArray", (void*)s);
						sv_magic(sv, rv, PERL_MAGIC_tiedscalar, Nullch, 0);
						hasMagic = true;
					}
				}
            } else {
				if( !m->type().isConst() ) {
					if(SvREADONLY(sv) && m->type().isPtr()) {
						m->item().s_voidp = 0;
						break;
					}
					s = new QByteArray(0);
					if( !SvREADONLY(sv) ) {
						SV* rv = newSV(0);
						sv_setpv_mg(sv, "");
						sv_setref_pv(rv, "Qt::_internal::QByteArray", s);
						sv_magic(sv, rv, PERL_MAGIC_tiedscalar, Nullch, 0);
						hasMagic = true;
					}
				} else
					s = new QByteArray(0);
			}
			m->item().s_voidp = s;
			m->next();
			if(s && !hasMagic && m->cleanup())
				delete s;
		}
		break;
		// ToSV is probably overkill here, but will do well as a template for other types.
		case Marshall::ToSV:
		{
			bool hasMagic = false;
			SV *sv = m->var();
			QByteArray *s = (QByteArray*)m->item().s_voidp;
			if(s) {
				if( !m->type().isConst() && !m->type().isStack() && !SvREADONLY(sv)) {
					SV* rv = newSV(0);
					sv_setref_pv(rv, "Qt::_internal::QByteArray", (void*)s); 
					sv_magic(sv, rv, PERL_MAGIC_tiedscalar, Nullch, 0); // err, is a previous magic auto-untied here?
					hasMagic = true;
				} else 
					sv_setpvn_mg(sv, (const char *)s->data(), s->size());
			} else
				sv_setsv_mg(sv, &PL_sv_undef);
			if(m->cleanup() && !hasMagic)
				delete s;
		}
		break;
		default:
			m->unsupported();
		break;
	}
}

static const char *not_ascii(const char *s, uint &len)
{
	bool r = false;
	for(; *s ; s++, len--)
		if((uint)*s > 0x7F)
		{
			r = true;
			break;
		}
	return r ? s : 0L;
}
/*
static void marshall_QCOORD_array(Marshall *m) {
	switch(m->action()) {
		case Marshall::FromSV:
		{
			SV *sv = m->var();
			if(!SvROK(sv) || SvTYPE(SvRV(sv)) != SVt_PVAV || av_len((AV*)SvRV(sv)) < 0) {
				m->item().s_voidp = 0;
				break;
			}
			AV *av = (AV*)SvRV(sv);
			int count = av_len(av);
			QCOORD *coord = new QCOORD[count + 2];
			for(int i = 0; i <= count; i++) {
				SV **svp = av_fetch(av, i, 0);
				coord[i] = svp ? SvIV(*svp) : 0;
			}
			m->item().s_voidp = coord;
			m->next();
		}
		break;
		default:
			m->unsupported();
	}
}
*/
static void marshall_intR(Marshall *m) {
	switch(m->action()) {
		case Marshall::FromSV:
		{
			SV *sv = m->var();
			if(m->type().isPtr() &&		// is pointer
			!SvOK(sv) && SvREADONLY(sv)) {   // and real undef
				m->item().s_voidp = 0;		// pass null pointer
				break;
			}
			if(m->cleanup()) {
				int i = SvIV(sv);
				m->item().s_voidp = &i;
				m->next();
				sv_setiv_mg(sv, (IV)i);
			} else {
				m->item().s_voidp = new int((int)SvIV(sv));
				if(PL_dowarn)
					warn("Leaking memory from int& handler");
			}
		}
		break;
		case Marshall::ToSV:
		{
			int *ip = (int*)m->item().s_voidp;
			SV *sv = m->var();
			if(!ip) {
				sv_setsv_mg(sv, &PL_sv_undef);
				break;
			}
			sv_setiv_mg(sv, *ip);
			m->next();
			if(!m->type().isConst())
				*ip = (int)SvIV(sv);
		}
		break;
		default:
			m->unsupported();
		break;
	}
}

static void marshall_boolR(Marshall *m) {
	switch(m->action()) {
		case Marshall::FromSV:
		{
			SV *sv = m->var();
			if(m->type().isPtr() &&		// is pointer
			!SvOK(sv) && SvREADONLY(sv)) {   // and real undef
				m->item().s_voidp = 0;		// pass null pointer
				break;
			}
			if(m->cleanup()) {
				bool i = SvTRUE(sv)? true : false;
				m->item().s_voidp = &i;
				m->next();
				sv_setsv_mg(sv, boolSV(i));
			} else {
				m->item().s_voidp = new bool(SvTRUE(sv)?true:false);
				if(PL_dowarn)
					warn("Leaking memory from bool& handler");
			}
		}
		break;
		case Marshall::ToSV:
		{
			bool *ip = (bool*)m->item().s_voidp;
			SV *sv = m->var();
			if(!ip) {
				sv_setsv_mg(sv, &PL_sv_undef);
				break;
			}
			sv_setsv_mg(sv, boolSV(*ip));
			m->next();
			if(!m->type().isConst())
				*ip = SvTRUE(sv)? true : false;
		}
		break;
		default:
			m->unsupported();
		break;
	}
}

static void marshall_charP_array(Marshall *m) {
	switch(m->action()) {
		case Marshall::FromSV:
		{
			SV *sv = m->var();
			if(!SvROK(sv) || SvTYPE(SvRV(sv)) != SVt_PVAV ||
				av_len((AV*)SvRV(sv)) < 0) {
				m->item().s_voidp = 0;
				break;
			}
	
			AV *arglist = (AV*)SvRV(sv);
			int count = av_len(arglist);
			char **argv = new char *[count + 2];
			int i;
			for(i = 0; i <= count; i++) {
				SV **item = av_fetch(arglist, i, 0);
				if(!item || !SvOK(*item)) {
					argv[i] = new char[1];
					argv[i][0] = 0;	// should undef warn?
					continue;
				}
	
				STRLEN len;
				char *s = SvPV(*item, len);
				argv[i] = new char[len + 1];
				strncpy(argv[i], s, len);
				argv[i][len] = 0;	// null terminazi? yes
			}
			argv[i] = 0;
			m->item().s_voidp = argv;
			m->next();
			if(m->cleanup()) {
				av_clear(arglist);
				for(i = 0; argv[i]; i++)
					av_push(arglist, newSVpv(argv[i], 0));
				delete [] argv;
			// perhaps we should check current_method?   
			}
		}
		break;
		default:
			m->unsupported();
		break;
	}
}

static void marshall_QStringList(Marshall *m) {
	switch(m->action()) {
		case Marshall::FromSV:
		{
			SV *sv = m->var();
			if(!SvROK(sv) || SvTYPE(SvRV(sv)) != SVt_PVAV ||
				av_len((AV*)SvRV(sv)) < 0) {
				m->item().s_voidp = 0;
				break;
			}
			AV *list = (AV*)SvRV(sv);
			int count = av_len(list);
			QStringList *stringlist = new QStringList;
			int i;
			COP *cop = cxstack[cxstack_ix].blk_oldcop;
			bool lc = cop->op_private & HINT_LOCALE;
			for(i = 0; i <= count; i++) {
				SV **item = av_fetch(list, i, 0);
				if(!item || !SvOK(*item)) {
					stringlist->append(QString());
					continue;
				}

				if(SvUTF8(*item))
					stringlist->append(QString::fromUtf8(SvPV_nolen(*item)));
				else if(lc)
					stringlist->append(QString::fromLocal8Bit(SvPV_nolen(*item))); 
				else
					stringlist->append(QString::fromLatin1(SvPV_nolen(*item)));
			}

			m->item().s_voidp = stringlist;
			m->next();

			if(m->cleanup()) {
				av_clear(list);
				COP *cop = cxstack[cxstack_ix].blk_oldcop;
				if(!(cop->op_private & HINT_BYTES))
					for(QStringList::Iterator it = stringlist->begin();
					it != stringlist->end();
					++it) {
						SV *sv = newSVpv((const char *)(*it).toUtf8(), 0);
						SvUTF8_on(sv);
						av_push(list, sv);
					}
				else if(cop->op_private & HINT_LOCALE)
					for(QStringList::Iterator it = stringlist->begin();
					it != stringlist->end();
					++it) {
						SV *sv = newSVpv((const char *)(*it).toLocal8Bit(), 0);
						av_push(list, sv);
					}
				else
					for(QStringList::Iterator it = stringlist->begin();
					it != stringlist->end();
					++it) {
						SV *sv = newSVpv((const char *)(*it).toLatin1(), 0);
						av_push(list, sv);
					}
				delete stringlist;
			}
		}
		break;
		case Marshall::ToSV:
		{
			QStringList *stringlist = (QStringList*)m->item().s_voidp;
			if(!stringlist) {
				sv_setsv_mg(m->var(), &PL_sv_undef);
				break;
			}
	
			AV *av = newAV();
			{
				SV *rv = newRV_noinc((SV*)av);
				sv_setsv_mg(m->var(), rv);
				SvREFCNT_dec(rv);
			}
			COP *cop = cxstack[cxstack_ix].blk_oldcop;
			if(!(cop->op_private & HINT_BYTES))
				for(QStringList::Iterator it = stringlist->begin();
				it != stringlist->end();
				++it) {
					SV *sv = newSVpv((const char *)(*it).toUtf8(), 0);
					SvUTF8_on(sv);
					av_push(av, sv);
				}
			else if(cop->op_private & HINT_LOCALE)
				for(QStringList::Iterator it = stringlist->begin();
				it != stringlist->end();
				++it) {
					SV *sv = newSVpv((const char *)(*it).toLocal8Bit(), 0);
					av_push(av, sv);
				}
			else
				for(QStringList::Iterator it = stringlist->begin();
				it != stringlist->end();
				++it) {
					SV *sv = newSVpv((const char *)(*it).toLatin1(), 0);
					av_push(av, sv);
				}
			if(m->cleanup())
				delete stringlist;
		}
		break;
		default:
			m->unsupported();
		break;
	}
}

static void marshall_QListInt(Marshall *m) {
	switch(m->action()) {
		case Marshall::FromSV:
		{
			SV *sv = m->var();
			if(!SvROK(sv) || SvTYPE(SvRV(sv)) != SVt_PVAV ||
				av_len((AV*)SvRV(sv)) < 0) {
				m->item().s_voidp = 0;
				break;
			}
			AV *list = (AV*)SvRV(sv);
			int count = av_len(list);
			QList<int> *valuelist = new QList<int>;
			int i;
			for(i = 0; i <= count; i++) {
				SV **item = av_fetch(list, i, 0);
				if(!item || !SvOK(*item)) {
					valuelist->append(0);
					continue;
				}
	
				valuelist->append(SvIV(*item));
			}
	
			m->item().s_voidp = valuelist;
			m->next();
	
			if(m->cleanup()) {
				av_clear(list);
				for(QList<int>::Iterator it = valuelist->begin();
				it != valuelist->end();
				++it)
					av_push(list, newSViv((int)*it));
				delete valuelist;
			}
		}
		break;
		case Marshall::ToSV:
		{
			QList<int> *valuelist = (QList<int>*)m->item().s_voidp;
			if(!valuelist) {
				sv_setsv_mg(m->var(), &PL_sv_undef);
				break;
			}
	
			AV *av = newAV();
			{
				SV *rv = newRV_noinc((SV*)av);
				sv_setsv_mg(m->var(), rv);
				SvREFCNT_dec(rv);
			}
	
			for(QList<int>::Iterator it = valuelist->begin();
			it != valuelist->end();
			++it)
				av_push(av, newSViv((int)*it));
			if(m->cleanup())
				delete valuelist;
		}
		break;
		default:
			m->unsupported();
		break;
	}
}

static void marshall_QListUInt(Marshall *m) {
	switch(m->action()) {
		case Marshall::FromSV:
		{
			SV *sv = m->var();
			if(!SvROK(sv) || SvTYPE(SvRV(sv)) != SVt_PVAV ||
				av_len((AV*)SvRV(sv)) < 0) {
				m->item().s_voidp = 0;
				break;
			}
			AV *list = (AV*)SvRV(sv);
			int count = av_len(list);
			QList<uint> *valuelist = new QList<uint>;
			int i;
			for(i = 0; i <= count; i++) {
				SV **item = av_fetch(list, i, 0);
				if(!item || !SvOK(*item)) {
					valuelist->append(0);
					continue;
				}
	
				valuelist->append(SvIV(*item));
			}
	
			m->item().s_voidp = valuelist;
			m->next();
	
			if(m->cleanup()) {
				av_clear(list);
				for(QList<uint>::Iterator it = valuelist->begin();
				it != valuelist->end();
				++it)
					av_push(list, newSViv((int)*it));
				delete valuelist;
			}
		}
		break;
		case Marshall::ToSV:
		{
			QList<uint> *valuelist = (QList<uint>*)m->item().s_voidp;
			if(!valuelist) {
				sv_setsv_mg(m->var(), &PL_sv_undef);
				break;
			}
	
			AV *av = newAV();
			{
				SV *rv = newRV_noinc((SV*)av);
				sv_setsv_mg(m->var(), rv);
				SvREFCNT_dec(rv);
			}
	
			for(QList<uint>::Iterator it = valuelist->begin();
			it != valuelist->end();
			++it)
				av_push(av, newSViv((int)*it));
			if(m->cleanup())
				delete valuelist;
		}
		break;
		default:
			m->unsupported();
		break;
	}
}

void marshall_voidP(Marshall *m) {
	switch(m->action()) {
		case Marshall::FromSV:
		{
			SV *sv = m->var();
			if(SvROK(sv) && SvRV(sv) && SvOK(SvRV(sv)))
				m->item().s_voidp = (void*)SvIV(SvRV(m->var()));
			else
				m->item().s_voidp = 0;
		}
		break;
		case Marshall::ToSV:
		{
			SV *sv = newSViv((IV)m->item().s_voidp);
			SV *rv = newRV_noinc(sv);
			sv_setsv_mg(m->var(), rv);
			SvREFCNT_dec(rv);
		}
		break;
		default:
			m->unsupported();
		break;
	}
}

void marshall_QRgb_array(Marshall *m) {
	switch(m->action()) {
		case Marshall::FromSV:
		{
			SV* sv = m->var();
			QRgb* s = 0;
			MAGIC* mg = 0;
			if( SvOK(sv) && SvTYPE(sv) == SVt_PVMG  && (mg = mg_find(sv, PERL_MAGIC_tiedscalar))
			&& sv_derived_from(mg->mg_obj, "Qt::_internal::QRgbStar") ) {
				s = (QRgb*)SvIV((SV*)SvRV(mg->mg_obj));
			} else if(!SvROK(sv) || SvREADONLY(sv) || SvTYPE(SvRV(sv)) != SVt_PVAV ||
			av_len((AV*)SvRV(sv)) < 0) {
				m->item().s_voidp = 0;
				break;
			} else {
				AV *list = (AV*)SvRV(sv);
				int count = av_len(list);
				s = new QRgb[count + 2];
				int i;
				for(i = 0; i <= count; i++) {
					SV **item = av_fetch(list, i, 0);
					if(!item || !SvOK(*item)) {
						s[i] = 0;
						continue;
					}
					s[i] = SvIV(*item);
				}
				s[i] = 0;
				SV* rv = newSV(0);
				sv_setref_pv(rv, "Qt::_internal::QRgbStar", (void*)s);
				sv_magic(sv, rv, PERL_MAGIC_tiedscalar, Nullch, 0);	    
			}
			m->item().s_voidp = s;
		}
		break;
		default:
			m->unsupported();
		break;
	}
}

// Templated classes marshallers

#define GET_PERL_OBJECT( CCLASS, PCLASS, IS_STACK )                \
      SV *sv = getPointerObject((void*)t);                         \
      SV *ret= newSV(0);                                           \
      if(!sv || !SvROK(sv)){                                       \
          HV *hv = newHV();                                        \
          SV *obj = newRV_noinc((SV*)hv);                          \
                                                                   \
          smokeperl_object o;                                      \
          o.smoke = m->smoke();                                    \
          o.classId = ix;                                          \
          o.ptr = (void*)t;                                        \
          o.allocated = IS_STACK;                                  \
                                                                   \
          sv_bless(obj, gv_stashpv( PCLASS, TRUE));                \
                                                                   \
          if(m->type().isConst() && m->type().isRef()) {           \
              void* p = construct_copy( &o );                      \
              if(p) {                                              \
                  o.ptr = p;                                       \
                  o.allocated = true;                              \
              }                                                    \
          }                                                        \
          sv_magic((SV*)hv, sv_qapp, '~', (char*)&o, sizeof(o));   \
          MAGIC *mg = mg_find((SV*)hv, '~');                       \
          mg->mg_virtual = &vtbl_smoke;                            \
                                                                   \
          sv_setsv_mg(ret, obj);                                   \
          SvREFCNT_dec(obj);                                       \
      }                                                            \
      else                                                         \
          sv_setsv_mg(ret, sv);

static int dateId, dateTimeId, timeId;
static bool idsResolved = false;

QVariant sv_to_var( SV * svitem )
{
	smokeperl_object * o = 0;
	if( SvIOK(svitem) ) {
		if( SvUOK(svitem) )
			return QVariant( (uint)SvUV(svitem) );
		else
			return QVariant( (int)SvIV(svitem) );
	} else if( SvPOK(svitem) ) {
		//warn( "Converting String to QVariant" );
		COP *cop = cxstack[cxstack_ix].blk_oldcop;
		bool lc = cop->op_private & HINT_LOCALE;
		if(SvUTF8(svitem))
			return QVariant( QString::fromUtf8(SvPV_nolen(svitem)) );
		else if(lc)
			return QVariant(QString::fromLocal8Bit(SvPV_nolen(svitem))); 
		else
			return QVariant(QString::fromLatin1(SvPV_nolen(svitem)));
	} else if( o=sv_obj_info(svitem) ) {
		if(!o->ptr) {
			return QVariant( QString::null );
		} else {
			void *ptr = o->ptr;
			if( o->classId == timeId )
				return QVariant( *((QTime*)ptr) );
			else if( o->classId == dateTimeId )
				return QVariant( *((QDateTime*)ptr) );
			else if( o->classId = dateId )
				return QVariant( *((QDate*)ptr) );
			else
				return QVariant( QString::null );
		}
	}
	return QVariant();
}

void marshall_QListQVariant( Marshall * m )
{
	Smoke * smoke = m->smoke();
	if( !idsResolved ) {
		dateId = smoke->idClass( "QDate" );
		dateTimeId = smoke->idClass( "QDateTime" );
		timeId = smoke->idClass( "QTime" );
		idsResolved = true;
	}

	switch( m->action() ) {
		case Marshall::FromSV:
		{
			SV *sv = m->var();
			
			QList<QVariant> *varlist = new QList<QVariant>;
			m->item().s_voidp = (void*)varlist;

			if( SvTYPE(SvRV(sv)) == SVt_PVAV ) {
				//warn("Converting list to VarList");
				if( av_len((AV*)SvRV(sv)) >= 0 ) {
					AV *list = (AV*)SvRV(sv);
					int count = av_len(list);
					int i;
					for(i = 0; i <= count; i++) {
						SV **item = av_fetch(list, i, 0);
						if(!item || !SvOK(*item)) {
							varlist->append( QVariant( QString::null ) );
							continue;
						}
						varlist->append( sv_to_var( *item ) );
					}
				}
			} else {
				//warn( "Converting sv to VarList" );
				varlist->append( sv_to_var( sv ) );
			}
			
			m->next();
			if( m->cleanup() ) {
				delete varlist;
			}
			break;
		}
		case Marshall::ToSV:
		{
			QList<QVariant> *vvlist = (QList<QVariant>*)m->item().s_voidp;
			if(!vvlist) {
				sv_setsv_mg(m->var(), &PL_sv_undef);
				break;
			}
	
			AV * av = newAV();
			{
				SV * rv = newRV_noinc((SV*)av);
				sv_setsv_mg(m->var(), rv);
				SvREFCNT_dec(rv);
			}
			
			for( QList<QVariant>::Iterator it = vvlist->begin(); it != vvlist->end(); ++it )
			{
				// No need for getPointerObject, because we just created this pointer
				QVariant & v = *it;
				
				int id = 0;
				void * voidp=0;
				char * cn=0;
				switch( v.type() ) {
					case QVariant::LongLong:
					case QVariant::Int:
					case QVariant::Bool:
					{
						SV * sv = newSViv( v.toInt() );
						av_push( av, sv );
					}
					break;
					case QVariant::ULongLong:
					case QVariant::UInt:
					{
						SV * sv = newSVuv( v.toUInt() );
						av_push( av, sv );
					}
					break;
					case QVariant::Double:
					{
						SV * sv = newSVnv( v.toDouble() );
						av_push( av, sv );
					}
					break;
					case QVariant::String:
					{
						COP *cop = cxstack[cxstack_ix].blk_oldcop;
						if(!(cop->op_private & HINT_BYTES)) {
							SV *sv = newSVpv((const char *)v.toString().toUtf8(), 0);
							SvUTF8_on(sv);
							av_push(av, sv);
						}
						else if(cop->op_private & HINT_LOCALE) {
							SV *sv = newSVpv((const char *)v.toString().toLocal8Bit(), 0);
							av_push(av, sv);
						}
						else {
							SV *sv = newSVpv((const char *)v.toString().toLatin1(), 0);
							av_push(av, sv);
						}
					}
					break;
					case QVariant::Date:
						voidp = (void*)(new QDate( v.toDate() ));
						id = dateId;
						cn = "Qt::Date";
						break;
					case QVariant::DateTime:
						voidp = (void*)(new QDateTime( v.toDateTime() ));
						id = dateTimeId;
						cn = "Qt::DateTime";
						break;
					case QVariant::Time:
						voidp = (void*)(new QTime( v.toTime() ));
						id = timeId;
						cn = "Qt::Time";
						break;
					default:
						warn( "Couldn't marshall QVariant type %s", v.typeName() );
						break;
				}
				
				if( voidp ) {
					SV *ret= newSV(0);
					HV *hv = newHV();
					SV *obj = newRV_noinc((SV*)hv);
					smokeperl_object o;
					o.smoke = smoke;
					o.classId = id;
					o.ptr = voidp;
					o.allocated = true;
					sv_bless(obj, gv_stashpv( cn, TRUE) );
					sv_magic((SV*)hv, sv_qapp, '~', (char*)&o, sizeof(o));
					MAGIC *mg = mg_find((SV*)hv, '~');
					mg->mg_virtual = &vtbl_smoke;
					sv_setsv_mg(ret, obj);
					av_push(av,ret);
					SvREFCNT_dec(obj);
				}
			}
			if( m->cleanup() )
				delete vvlist;
			break;
		}
		default:
			break;
    }
}

template<class T> void marshall_QList(Marshall * m, char * name, char * perl_name)
{
	Smoke * smoke = m->smoke();
	static int id = -1;
	if( id == -1 )
		id = smoke->idClass(name);

	switch( m->action() ) {
		case Marshall::FromSV:
		{
			break;
		}
		case Marshall::ToSV:
		{
			T * fl = (T*)m->item().s_voidp;
			if(!fl) {
				sv_setsv_mg(m->var(), &PL_sv_undef);
				break;
			}
	
			AV * av = newAV();
			{
				SV * rv = newRV_noinc((SV*)av);
				sv_setsv_mg(m->var(), rv);
				SvREFCNT_dec(rv);
			}

			for( typename T::iterator it = fl->begin(); it != fl->end(); ++it )
			{
				SV *ret= newSV(0);
				HV *hv = newHV();
				SV *obj = newRV_noinc((SV*)hv);
				smokeperl_object o;
				o.smoke = smoke;
				o.classId = id;
				o.ptr = *it;
				o.allocated = false;
				sv_bless(obj, gv_stashpv( perl_name, TRUE) );
				sv_magic((SV*)hv, sv_qapp, '~', (char*)&o, sizeof(o));
				MAGIC *mg = mg_find((SV*)hv, '~');
				mg->mg_virtual = &vtbl_smoke;
				sv_setsv_mg(ret, obj);
				av_push(av,ret);
				SvREFCNT_dec(obj);
			}
			
			if( m->cleanup() )
				delete fl;
			break;
		}
		default:
			break;
    }	
}

void marshall_FieldList( Marshall * m )
{
	marshall_QList<FieldList>( m, "Field", "Blur::Model::Field" );
}

void marshall_TableList( Marshall * m )
{
	marshall_QList<TableList>( m, "Table", "Blur::Model::Table" );
}

void marshall_IndexList( Marshall * m )
{
	marshall_QList<IndexList>( m, "Index", "Blur::Model::Index" );
}

void marshall_TableSchemaList( Marshall * m )
{
	marshall_QList<TableSchemaList>( m, "TableSchema", "Blur::Model::TableSchema" );
}


void marshall_IndexSchemaList( Marshall * m )
{
	marshall_QList<IndexSchemaList>( m, "IndexSchema", "Blur::Model::IndexSchema" );
}


void marshall_QVariant(Marshall * m)
{
	Smoke * smoke = m->smoke();
	int dateId = smoke->idClass( "QDate" );
	int dateTimeId = smoke->idClass( "QDateTime" );
	int timeId = smoke->idClass( "QTime" );
	switch( m->action() ) {
		case Marshall::FromSV:
		{
			SV *sv = m->var();
			
			smokeperl_object * o = 0;
			if( !SvOK(sv) ) {
				m->item().s_voidp = new QVariant( QString::null );
			}else if( SvIOK(sv) ) {
				if( SvUOK(sv) )
					m->item().s_voidp = new QVariant( (uint)SvUV(sv) );
				else
					m->item().s_voidp = new QVariant( (int)SvIV(sv) );
			} else if( SvNOK(sv) ) {
				m->item().s_voidp = new QVariant( (double)SvNV(sv) );
			} else if( SvPOK(sv) ) {
				COP *cop = cxstack[cxstack_ix].blk_oldcop;
				bool lc = cop->op_private & HINT_LOCALE;
				if(SvUTF8(sv))
					m->item().s_voidp = new QVariant( QString::fromUtf8(SvPV_nolen(sv)) );
				else if(lc)
					m->item().s_voidp = new QVariant( QString::fromLocal8Bit(SvPV_nolen(sv)) ); 
				else
					m->item().s_voidp = new QVariant( QString::fromLatin1(SvPV_nolen(sv)) );
			} else if( o = sv_obj_info(sv) ) {
				if(!o->ptr) {
					m->item().s_voidp = 0;
				} else {
					void *ptr = o->ptr;
					if( o->classId == timeId )
						m->item().s_voidp = new QVariant( *((QTime*)ptr) );
					else if( o->classId == dateTimeId )
						m->item().s_voidp = new QVariant( *((QDateTime*)ptr) );
					else if( o->classId = dateId )
						m->item().s_voidp = new QVariant( *((QDate*)ptr) );
					else
						m->item().s_voidp = 0;
				}
			}
			m->next();
			if( m->cleanup() && m->item().s_voidp ) {
				delete (QVariant*)m->item().s_voidp;
			}
			break;
		}
		case Marshall::ToSV:
		{
			QVariant *var = (QVariant*)m->item().s_voidp;
			if(!var) {
				warn( "QVariant Pointer is null" );
				sv_setsv_mg(m->var(), &PL_sv_undef);
				break;
			}
	
			int id = 0;
			void * voidp=0;
			char * cn=0;
			//warn( var->typeName() );
			switch( var->type() ) {
				case QVariant::Int:
				case QVariant::LongLong:
				case QVariant::Bool:
					sv_setiv_mg(m->var(), (IV)var->toInt() );
					break;
				case QVariant::UInt:
				case QVariant::ULongLong:
					sv_setuv_mg(m->var(), (UV)var->toUInt() );
					break;
				case QVariant::Double:
					sv_setnv_mg(m->var(), (NV)var->toDouble() );
					break;
				case QVariant::String:
					{
						COP *cop = cxstack[cxstack_ix].blk_oldcop;
						if(!(cop->op_private & HINT_BYTES)) {
							sv_setpv_mg(m->var(), (const char *)var->toString().toUtf8());
							SvUTF8_on(m->var());
						}
						else if(cop->op_private & HINT_LOCALE) {
							sv_setpv_mg(m->var(), (const char *)var->toString().toLocal8Bit());
						}
						else {
							sv_setpv_mg(m->var(), (const char *)var->toString().toLatin1());
						}
					}
					break;
				case QVariant::Date:
					voidp = (void*)(new QDate( var->toDate() ));
					id = dateId;
					cn = "Qt::Date";
					break;
				case QVariant::DateTime:
					voidp = (void*)(new QDateTime( var->toDateTime() ));
					id = dateTimeId;
					cn = "Qt::DateTime";
					break;
				case QVariant::Time:
					voidp = (void*)(new QTime( var->toTime() ));
					id = timeId;
					cn = "Qt::Time";
					break;
				default:
					warn( "Couldn't marshall QVariant type %s", var->typeName() );
					break;
			}
				
			if( voidp ) {
				HV *hv = newHV();
				SV *obj = newRV_noinc((SV*)hv);
				smokeperl_object o;
				o.smoke = smoke;
				o.classId = id;
				o.ptr = voidp;
				o.allocated = true;
				sv_bless(obj, gv_stashpv( cn, TRUE) );
				sv_magic((SV*)hv, sv_qapp, '~', (char*)&o, sizeof(o));
				MAGIC *mg = mg_find((SV*)hv, '~');
				mg->mg_virtual = &vtbl_smoke;
				sv_setsv_mg(m->var(), obj);
				SvREFCNT_dec(obj);
			}
			if( m->cleanup() )
				delete var;
			break;
		}
		default:
			break;
    }
}

TypeHandler Qt_handlers[] = {
	{ "QString", marshall_QString },
	{ "QString&", marshall_QString },
	{ "QString*", marshall_QString },
	{ "const QString", marshall_QString },
	{ "const QString&", marshall_QString },
	{ "const QString*", marshall_QString },
	{ "QStringList", marshall_QStringList },
	{ "QStringList&", marshall_QStringList },
	{ "QStringList*", marshall_QStringList },
	{ "int&", marshall_intR },
	{ "int*", marshall_intR },
	{ "bool&", marshall_boolR },
	{ "bool*", marshall_boolR },
	{ "char*", marshall_charP },
	{ "const char*", marshall_charP },
	{ "char**", marshall_charP_array },
	{ "uchar*", marshall_ucharP },
	{ "QRgb*", marshall_QRgb_array },
	{ "QUObject*", marshall_voidP },
//	{ "const QCOORD*", marshall_QCOORD_array },
	{ "void", marshall_void },
	{ "QByteArray", marshall_QByteArray },
	{ "QByteArray&", marshall_QByteArray },
	{ "QByteArray*", marshall_QByteArray },
	{ "QList<int>", marshall_QListInt },
	{ "QList<int>*", marshall_QListInt },
	{ "QList<int>&", marshall_QListInt },
	{ "QList<uint>", marshall_QListUInt },
	{ "QList<uint>*", marshall_QListUInt },
	{ "QList<uint>&", marshall_QListUInt },    
	{ "QList<QVariant>", marshall_QListQVariant },
	{ "QList<QVariant>*", marshall_QListQVariant },
	{ "QList<QVariant>&", marshall_QListQVariant },
	{ "VarList&", marshall_QListQVariant },
	{ "VarList", marshall_QListQVariant },
	{ "QVariant", marshall_QVariant },
	{ "QVariant*", marshall_QVariant },
	{ "QVariant&", marshall_QVariant },
	{ "FieldList", marshall_FieldList },
	{ "FieldList*", marshall_FieldList },
	{ "FieldList&", marshall_FieldList },
	{ "QList<Field*>", marshall_FieldList },
	{ "QList<Field*>*", marshall_FieldList },
	{ "QList<Field*>&", marshall_FieldList },
	{ "TableList", marshall_TableList },
	{ "TableList*", marshall_TableList },
	{ "TableList&", marshall_TableList },
	{ "QList<Table*>", marshall_TableList },
	{ "QList<Table*>*", marshall_TableList },
	{ "QList<Table*>&", marshall_TableList },
	{ "IndexList", marshall_IndexList },
	{ "IndexList*", marshall_IndexList },
	{ "IndexList&", marshall_IndexList },
	{ "QList<Index*>", marshall_IndexList },
	{ "QList<Index*>*", marshall_IndexList },
	{ "QList<Index*>&", marshall_IndexList },
	{ "TableSchemaList", marshall_TableSchemaList },
	{ "TableSchemaList*", marshall_TableSchemaList },
	{ "TableSchemaList&", marshall_TableSchemaList },
	{ "QList<TableSchema*>", marshall_TableSchemaList },
	{ "QList<TableSchema*>*", marshall_TableSchemaList },
	{ "QList<TableSchema*>&", marshall_TableSchemaList },
	{ "IndexSchemaList", marshall_IndexSchemaList },
	{ "IndexSchemaList*", marshall_IndexSchemaList },
	{ "IndexSchemaList&", marshall_IndexSchemaList },
	{ "QList<IndexSchema*>", marshall_IndexSchemaList },
	{ "QList<IndexSchema*>*", marshall_IndexSchemaList },
	{ "QList<IndexSchema*>&", marshall_IndexSchemaList },
    { 0, 0 }
};

static HV *type_handlers = 0;

void install_handlers(TypeHandler *h) {
    if(!type_handlers) type_handlers = newHV();
    while(h->name) {
	hv_store(type_handlers, h->name, strlen(h->name), newSViv((IV)h), 0);
	h++;
    }
    if(!dtorcache){
	 dtorcache = new QHash<int,Smoke::Index*>();
//	 dtorcache->setAutoDelete(1);
    }
    if(!cctorcache) {
	cctorcache = new QHash<int,Smoke::Index*>();
//	cctorcache->setAutoDelete(1);
    }
}

Marshall::HandlerFn getMarshallFn(const SmokeType &type) {
    if(type.elem())
	return marshall_basetype;
    if(!type.name())
	return marshall_void;
    if(!type_handlers) {
	return marshall_unknown;
    }
    U32 len = strlen(type.name());
    SV **svp = hv_fetch(type_handlers, type.name(), len, 0);
    if(!svp && type.isConst() && len > 6)
	svp = hv_fetch(type_handlers, type.name() + 6, len - 6, 0);
    if(svp) {
	TypeHandler *h = (TypeHandler*)SvIV(*svp);
	return h->fn;
    }
    return marshall_unknown;
}
