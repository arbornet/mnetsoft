/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* SYSTEM DICTIONARY - This is the table that stores all backtalk built-in
 * functions and variables.
 */

#include <stdio.h>
#include <ctype.h>
#include "backtalk.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define D(n,f,t,i) {n, "D" #f "," #t "," #i},
#define F(n,f,t,i) {n, "F" #f "," #t "," #i},
#define E(n,f,t,i) {n, "E" #f "," #t "," #i},

struct { char *n; char *v; } dict[]= {

    F("!",		DT_CONSTANT, TK_FUNCTION, func_not)
    F("'",		DT_CONSTANT, TK_FUNCTION, func_concatinate)
    F("*",		DT_CONSTANT, TK_FUNCTION, func_mul)
    F("+",		DT_CONSTANT, TK_FUNCTION, func_add)
    F("-",		DT_CONSTANT, TK_FUNCTION, func_sub)
    F("/",		DT_CONSTANT, TK_FUNCTION, func_div)
    D("[",		DT_CONSTANT, TK_MARK,     0)
    F("]",		DT_CONSTANT, TK_FUNCTION, func_mkarray)
    D("`",		DT_CONSTANT, TK_MARK,	  1)
    F("abs",		DT_CONSTANT, TK_FUNCTION, func_abs)
    D("alias",		0,	     TK_STRING,   "")
    D("allowanon",	DT_CONSTANT, TK_INTEGER,  1)
    D("allowgraphics",	0,           TK_INTEGER,  1)
    F("aload",		DT_CONSTANT, TK_FUNCTION, func_aload)
    D("amadm",		DT_PROTECT,  TK_INTEGER,  0)
    D("amfw",		DT_PROTECT,  TK_INTEGER,  0)
    F("and",		DT_CONSTANT, TK_FUNCTION, func_and)
    D("anonymity",	DT_CONSTANT, TK_INTEGER,  0)
    F("asort",		DT_CONSTANT, TK_FUNCTION, func_asort)
#if ATTACHMENTS
    D("attachments",	0,	     TK_STRING,   "")
#else
    D("attachments",	DT_CONSTANT, TK_STRING,   "")
#endif
    D("author_edit",	DT_CONSTANT, TK_INTEGER,  0)
    D("author_erase",	DT_CONSTANT, TK_INTEGER,  1)
    D("author_freeze",	DT_CONSTANT, TK_INTEGER,  1)
    D("author_hide",	DT_CONSTANT, TK_INTEGER,  1)
    D("author_kill",	DT_CONSTANT, TK_INTEGER,  1)
    D("author_retire",	DT_CONSTANT, TK_INTEGER,  1)
    D("author_retitle",	DT_CONSTANT, TK_INTEGER,  1)
    D("authorid",	0,	     TK_STRING,   "")
    D("authorname",	0,	     TK_STRING,   "")
    D("authoruid",	0,	     TK_INTEGER,  0)
    D("auto_recompile",	DT_PROTECT,  TK_INTEGER,  1)
    F("backtalkurl",	DT_CONSTANT, TK_FUNCTION, func_backtalkurl)
    F("band",		DT_CONSTANT, TK_FUNCTION, func_band)
    D("bbsdir",		DT_CONSTANT, TK_STRING,   BBS_DIR)
    F("begin",		DT_CONSTANT, TK_FUNCTION, func_begin)
    D("blindfold",	0,	     TK_INTEGER,  0)
    F("bor",		DT_CONSTANT, TK_FUNCTION, func_bor)
    F("break",		DT_CONSTANT, TK_FUNCTION, func_break)
    F("browser",	DT_CONSTANT, TK_FUNCTION, func_browser)
    F("cache_conflist",	DT_CONSTANT, TK_FUNCTION, func_cache_conflist)
    F("call",		DT_CONSTANT, TK_FUNCTION, func_call)
    D("canattach",	DT_CONSTANT, TK_INTEGER,  ATTACHMENTS)
    D("canemail",	DT_CONSTANT, TK_INTEGER,  EMAIL)
    D("canspell",	DT_CONSTANT, TK_INTEGER,  SPELLCHECK)
    F("cap",		DT_CONSTANT, TK_FUNCTION, func_cap)
    F("caps",		DT_CONSTANT, TK_FUNCTION, func_caps)
    F("cflist_array",	DT_CONSTANT, TK_FUNCTION, func_cflist_array)
    D("cgiquery",	DT_PROTECT,  TK_STRING,   "")
    F("cgiquote",	DT_CONSTANT, TK_FUNCTION, func_cgiquote)
    F("chain",		DT_CONSTANT, TK_FUNCTION, func_chain)
    F("changeemail",	DT_CONSTANT, TK_FUNCTION, func_changeemail)
    F("changegroup",	DT_CONSTANT, TK_FUNCTION, func_changegroup)
    F("changename",	DT_CONSTANT, TK_FUNCTION, func_changename)
    F("changepass",	DT_CONSTANT, TK_FUNCTION, func_changepass)
    F("check_partic",	DT_CONSTANT, TK_FUNCTION, func_check_partic)
    F("chomp",		DT_CONSTANT, TK_FUNCTION, func_chomp)
    F("cleanhtml",	DT_CONSTANT, TK_FUNCTION, func_cleanhtml)
    F("clear",		DT_CONSTANT, TK_FUNCTION, func_clear)
    F("cleartomark",	DT_CONSTANT, TK_FUNCTION, func_cleartomark)
    F("clip",		DT_CONSTANT, TK_FUNCTION, func_clip)
    F("close_conf",	DT_CONSTANT, TK_FUNCTION, func_close_conf)
    D("conf",		0,	     TK_STRING,   "")
    F("conf_alias",	DT_CONSTANT, TK_FUNCTION, func_conf_alias)
    F("conf_bull",	DT_CONSTANT, TK_FUNCTION, func_conf_bull)
    F("conf_dir",	DT_CONSTANT, TK_FUNCTION, func_conf_dir)
    F("conf_index",	DT_CONSTANT, TK_FUNCTION, func_conf_index)
    F("conf_login",	DT_CONSTANT, TK_FUNCTION, func_conf_login)
    F("conf_logout",	DT_CONSTANT, TK_FUNCTION, func_conf_logout)
    F("conf_new",	DT_CONSTANT, TK_FUNCTION, func_conf_new)
    D("confdir",	0,           TK_STRING,	  "")
    D("conflist",	DT_CONSTANT, TK_STRING,   CONFLIST)
    D("conftitle",	0,	     TK_STRING,   "")
    F("constant",	DT_CONSTANT, TK_FUNCTION, func_constant)
    F("continue",	DT_CONSTANT, TK_FUNCTION, func_continue)
    F("copy",		DT_CONSTANT, TK_FUNCTION, func_copy)
    D("copyright",	DT_CONSTANT, TK_STRING,   COPYRIGHT_STRING)
    F("count",		DT_CONSTANT, TK_FUNCTION, func_count)
    F("count_sel",	DT_CONSTANT, TK_FUNCTION, func_count_sel)
    F("counttomark",	DT_CONSTANT, TK_FUNCTION, func_counttomark)
    D("cpass",		0,	     TK_STRING,	  "")
    D("csel",		DT_MULTI,    TK_STRING,	  "")
    F("ctime",		DT_CONSTANT, TK_FUNCTION, func_ctime)
    F("cvi",		DT_CONSTANT, TK_FUNCTION, func_cvi)
    F("cvn",		DT_CONSTANT, TK_FUNCTION, func_cvn)
    F("cvs",		DT_CONSTANT, TK_FUNCTION, func_cvs)
    F("cvscol",		DT_CONSTANT, TK_FUNCTION, func_cvscol)
    F("cvt",		DT_CONSTANT, TK_FUNCTION, func_cvt)
    D("date",		0,	     TK_TIME,     0)
    D("dbtype",		DT_CONSTANT, TK_STRING,   "")
    F("dec",		DT_CONSTANT, TK_FUNCTION, func_dec)
    F("def",		DT_CONSTANT, TK_FUNCTION, func_def)
    F("default",	DT_CONSTANT, TK_FUNCTION, func_default)
    F("defconstant",	DT_CONSTANT, TK_FUNCTION, func_defconstant)
    F("defined",	DT_CONSTANT, TK_FUNCTION, func_defined)
    E("del_conf",	DT_CONSTANT, LIB_CNFADM,  func_del_conf)
    F("delgroup",	DT_CONSTANT, TK_FUNCTION, func_delgroup)
    F("delgroupmem",	DT_CONSTANT, TK_FUNCTION, func_delgroupmem)
    F("dfltgroup",	DT_CONSTANT, TK_FUNCTION, func_dfltgroup)
    F("dict",		DT_CONSTANT, TK_FUNCTION, func_dict)
    F("directory",	DT_CONSTANT, TK_FUNCTION, func_directory)
    F("dtime",		DT_CONSTANT, TK_FUNCTION, func_dtime)
    F("dumpstack",	DT_CONSTANT, TK_FUNCTION, func_dumpstack)
    F("dup",		DT_CONSTANT, TK_FUNCTION, func_dup)
    E("edit_attach",	DT_CONSTANT, LIB_CNFADM,  func_edit_attach)
    E("edit_resp",	DT_CONSTANT, LIB_CNFADM,  func_edit_resp)
    D("editdate",	0,	     TK_TIME,     0)
    D("editfrozen",	DT_CONSTANT, TK_INTEGER,  0)
    F("email_body",	DT_CONSTANT, TK_FUNCTION, func_email_body)
    F("email_close",	DT_CONSTANT, TK_FUNCTION, func_email_close)
    F("email_new",	DT_CONSTANT, TK_FUNCTION, func_email_new)
    F("email_recip",	DT_CONSTANT, TK_FUNCTION, func_email_recip)
    F("email_send",	DT_CONSTANT, TK_FUNCTION, func_email_send)
    F("end",		DT_CONSTANT, TK_FUNCTION, func_end)
    F("eq",		DT_CONSTANT, TK_FUNCTION, func_eq)
    E("erase_resp",	DT_CONSTANT, LIB_CNFADM,  func_erase_resp)
    D("erased",		0,	     TK_INTEGER,  0)
    F("exch",		DT_CONSTANT, TK_FUNCTION, func_exch)
    F("exec",		DT_CONSTANT, TK_FUNCTION, func_exec)
    D("exec_limit",	0,	     TK_INTEGER,  0)
    F("exists",		DT_CONSTANT, TK_FUNCTION, func_exists)
    F("expand",		DT_CONSTANT, TK_FUNCTION, func_expand)
    D("expect_resp",	0,           TK_INTEGER,  0)
    D("expire_session",	DT_CONSTANT, TK_INTEGER,  3600)
    F("fav_sel",	DT_CONSTANT, TK_FUNCTION, func_fav_sel)
    D("favicon",	0,	     TK_STRING,	  "")
    F("filedate",	DT_CONSTANT, TK_FUNCTION, func_filedate)
    F("first_partic",	DT_CONSTANT, TK_FUNCTION, func_first_partic)
    F("firstdir",	DT_CONSTANT, TK_FUNCTION, func_firstdir)
    F("firstuser",	DT_CONSTANT, TK_FUNCTION, func_firstuser)
    D("fishbowl",	DT_PROTECT,  TK_INTEGER,  0)
    D("flavor",		DT_CONSTANT, TK_STRING,	  "")
    F("for",		DT_CONSTANT, TK_FUNCTION, func_for)
    F("forall",		DT_CONSTANT, TK_FUNCTION, func_forall)
    F("forget_item",	DT_CONSTANT, TK_FUNCTION, func_forget_item)
    D("forgotten",	0,           TK_INTEGER,  0)
    E("freeze_item",	DT_CONSTANT, LIB_CNFADM,  func_freeze_item)
    D("frozen",		0,           TK_INTEGER,  0)
    D("fw_edit",	DT_CONSTANT, TK_INTEGER,  0)
    D("fw_erase",	DT_CONSTANT, TK_INTEGER,  1)
    D("fw_hide",	DT_CONSTANT, TK_INTEGER,  1)
    D("fw_retitle",	DT_CONSTANT, TK_INTEGER,  1)
    D("fwlist",		DT_PROTECT,  TK_STRING,	  "")
    F("ge",		DT_CONSTANT, TK_FUNCTION, func_ge)
    F("get",		DT_CONSTANT, TK_FUNCTION, func_get)
    F("getcookie",	DT_CONSTANT, TK_FUNCTION, func_getcookie)
    F("get_attach",	DT_CONSTANT, TK_FUNCTION, func_get_attach)
    F("get_conf_note",	DT_CONSTANT, TK_FUNCTION, func_get_conf_note)
    F("get_item_note",	DT_CONSTANT, TK_FUNCTION, func_get_item_note)
    D("gid",		DT_PROTECT,  TK_INTEGER,  -1)
    F("grep",		DT_CONSTANT, TK_FUNCTION, func_grep)
    F("groupid",	DT_CONSTANT, TK_FUNCTION, func_groupid)
    F("groupname",	DT_CONSTANT, TK_FUNCTION, func_groupname)
    D("grouplist",	DT_PROTECT,  TK_INTEGER,  0)
    F("groups",		DT_CONSTANT, TK_FUNCTION, func_groups)
    F("gt",		DT_CONSTANT, TK_FUNCTION, func_gt)
    F("halt",		DT_CONSTANT, TK_FUNCTION, func_halt)
    D("hidden",		0,	     TK_INTEGER,  0)
    E("hide_resp",	DT_CONSTANT, LIB_CNFADM,  func_hide_resp)
    D("homedir",	DT_PROTECT,  TK_STRING,   "")
    F("htime",		DT_CONSTANT, TK_FUNCTION, func_htime)
    D("http_content_type",0,	     TK_STRING,   "text/html")
    D("http_expires",	0,	     TK_TIME,	  0)
    D("http_headers",	0,	     TK_INTEGER,  1)
    D("http_location",	0,	     TK_STRING,   "")
    D("http_no_cache",	0,	     TK_INTEGER,   0)
    D("id",		DT_PROTECT,  TK_STRING,   "")
    F("if",		DT_CONSTANT, TK_FUNCTION, func_if)
    D("ifav",		0,           TK_INTEGER,  1)
    F("ifelse",		DT_CONSTANT, TK_FUNCTION, func_ifelse)
    F("in",		DT_CONSTANT, TK_FUNCTION, func_in)
    F("in_sel",		DT_CONSTANT, TK_FUNCTION, func_in_sel)
    F("inc",		DT_CONSTANT, TK_FUNCTION, func_inc)
    F("include",	DT_CONSTANT, TK_FUNCTION, func_include)
    F("index",		DT_CONSTANT, TK_FUNCTION, func_index)
    F("ingroup",	DT_CONSTANT, TK_FUNCTION, func_ingroup)
    F("inlist",		DT_CONSTANT, TK_FUNCTION, func_inlist)
    D("isel",		DT_MULTI,    TK_STRING,   "")
    D("item",		0,	     TK_INTEGER,  0)
    F("itime",		DT_CONSTANT, TK_FUNCTION, func_itime)
    F("jointomark",	DT_CONSTANT, TK_FUNCTION, func_jointomark)
    F("jsquote",	DT_CONSTANT, TK_FUNCTION, func_jsquote)
    F("jump",		DT_CONSTANT, TK_FUNCTION, func_jump)
    E("kill_item",	DT_CONSTANT, LIB_CNFADM,  func_kill_item)
    F("killsession",	DT_CONSTANT, TK_FUNCTION, func_killsession)
    F("known",		DT_CONSTANT, TK_FUNCTION, func_known)
    F("last_in_conf",	DT_CONSTANT, TK_FUNCTION, func_last_in_conf)
    D("lastdate",	0,	     TK_TIME,     0)
    F("le",		DT_CONSTANT, TK_FUNCTION, func_le)
    F("length",		DT_CONSTANT, TK_FUNCTION, func_length)
    F("line",		DT_CONSTANT, TK_FUNCTION, func_line)
    E("link_item",	DT_CONSTANT, LIB_CNFADM,  func_link_item)
    D("linkdate",	0,	     TK_TIME,     0)
    D("linked",		0,	     TK_INTEGER,  0)
    D("linktonew",	DT_CONSTANT, TK_INTEGER,  1)
    F("loadvar",	DT_CONSTANT, TK_FUNCTION, func_loadvar)
    D("loginlen",	DT_CONSTANT, TK_INTEGER,  MAX_LOGIN_LEN)
    D("loglevel",	DT_CONSTANT, TK_INTEGER,  0)
    F("loop",		DT_CONSTANT, TK_FUNCTION, func_loop)
    F("lt",		DT_CONSTANT, TK_FUNCTION, func_lt)
    F("make_attach",	DT_CONSTANT, TK_FUNCTION, func_make_attach)
    D("mark",		DT_CONSTANT, TK_MARK,     0)
    F("mark_unseen",	DT_CONSTANT, TK_FUNCTION, func_mark_unseen)
    D("maxitem",	0,	     TK_INTEGER,  0)
    D("maxread",	0,	     TK_INTEGER,  0)
    D("maxresp",	0,           TK_INTEGER,  0)
    D("mayedit",	DT_PROTECT,  TK_INTEGER,  0)
    D("mayerase",	DT_PROTECT,  TK_INTEGER,  0)
    D("mayfreeze",	DT_PROTECT,  TK_INTEGER,  0)
    D("mayhide",	DT_PROTECT,  TK_INTEGER,  0)
    D("maykill",	DT_PROTECT,  TK_INTEGER,  0)
    D("maypost",	DT_PROTECT,  TK_INTEGER,  0)
    D("mayread",	DT_PROTECT,  TK_INTEGER,  0)
    D("mayresp",	DT_PROTECT,  TK_INTEGER,  0)
    D("mayretire",	DT_PROTECT,  TK_INTEGER,  0)
    D("mayretitle",	DT_PROTECT,  TK_INTEGER,  0)
    D("mayseealias",	DT_PROTECT,  TK_INTEGER,  1)
    D("mayseefname",	DT_PROTECT,  TK_INTEGER,  1)
    F("mimename",	DT_CONSTANT, TK_FUNCTION, func_mimename)
    F("mod",		DT_CONSTANT, TK_FUNCTION, func_mod)
    F("ne",		DT_CONSTANT, TK_FUNCTION, func_ne)
    F("neg",		DT_CONSTANT, TK_FUNCTION, func_neg)
    E("new_conf",	DT_CONSTANT, LIB_CNFADM,  func_new_conf)
    D("newbie",		0,           TK_INTEGER,  0)
    F("newgroup",	DT_CONSTANT, TK_FUNCTION, func_newgroup)
    F("newgroupmem",	DT_CONSTANT, TK_FUNCTION, func_newgroupmem)
    F("newsession",	DT_CONSTANT, TK_FUNCTION, func_newsession)
    F("newuser",	DT_CONSTANT, TK_FUNCTION, func_newuser)
    D("newuseropen",	DT_CONSTANT, TK_INTEGER,  2)
    F("next_conf",	DT_CONSTANT, TK_FUNCTION, func_next_conf)
    F("next_int",	DT_CONSTANT, TK_FUNCTION, func_next_int)
    F("next_item",	DT_CONSTANT, TK_FUNCTION, func_next_item)
    F("next_partic",	DT_CONSTANT, TK_FUNCTION, func_next_partic)
    F("next_resp",	DT_CONSTANT, TK_FUNCTION, func_next_resp)
    F("nextdir",	DT_CONSTANT, TK_FUNCTION, func_nextdir)
    F("nextuser",	DT_CONSTANT, TK_FUNCTION, func_nextuser)
    D("nopwedit",	DT_CONSTANT, TK_INTEGER,  NOPWEDIT)
    D("noskip",		0,           TK_INTEGER,  0)
    F("ogrep",		DT_CONSTANT, TK_FUNCTION, func_ogrep)
    F("open_conf",	DT_CONSTANT, TK_FUNCTION, func_open_conf)
    F("or",		DT_CONSTANT, TK_FUNCTION, func_or)
    D("parentresp",	0,	     TK_INTEGER,  0)
    F("parse",		DT_CONSTANT, TK_FUNCTION, func_parse)
    D("particip",	DT_PROTECT,  TK_STRING,   "")
    F("pathname",	DT_CONSTANT, TK_FUNCTION, func_pathname)
    F("pop",		DT_CONSTANT, TK_FUNCTION, func_pop)
    F("post_item",	DT_CONSTANT, TK_FUNCTION, func_post_item)
    D("post_log_file",	DT_CONSTANT, TK_STRING,   "")
    F("post_resp",	DT_CONSTANT, TK_FUNCTION, func_post_resp)
    F("pr",		DT_CONSTANT, TK_FUNCTION, func_pr)
    F("print",		DT_CONSTANT, TK_FUNCTION, func_print)
    D("progname",	DT_PROTECT,  TK_STRING,   "")
    D("publicflavors",	DT_CONSTANT, TK_STRING,   "public")
    F("put",		DT_CONSTANT, TK_FUNCTION, func_put)
    F("quote",		DT_CONSTANT, TK_FUNCTION, func_quote)
    F("rand",		DT_CONSTANT, TK_FUNCTION, func_rand)
    F("read",		DT_CONSTANT, TK_FUNCTION, func_read)
    F("read_item",	DT_CONSTANT, TK_FUNCTION, func_read_item)
    F("read_resp",	DT_CONSTANT, TK_FUNCTION, func_read_resp)
    F("read_text",	DT_CONSTANT, TK_FUNCTION, func_read_text)
    F("readable",	DT_CONSTANT, TK_FUNCTION, func_readable)
    D("readdate",	0,           TK_TIME,     0)
    E("reattach",	DT_CONSTANT, LIB_CNFADM,  func_reattach)
    F("regex",		DT_CONSTANT, TK_FUNCTION, func_regex)
    F("remember_item",	DT_CONSTANT, TK_FUNCTION, func_remember_item)
    F("removeuser",	DT_CONSTANT, TK_FUNCTION, func_removeuser)
    F("repeat",		DT_CONSTANT, TK_FUNCTION, func_repeat)
    F("replace",	DT_CONSTANT, TK_FUNCTION, func_replace)
    F("resign_conf",	DT_CONSTANT, TK_FUNCTION, func_resign_conf)
    D("resp",		0,	     TK_INTEGER,  0)
    E("retire_item",	DT_CONSTANT, LIB_CNFADM,  func_retire_item)
    D("retired",	0,           TK_INTEGER,  0)
    E("retitle_item",	DT_CONSTANT, LIB_CNFADM,  func_retitle_item)
    F("rev_sel",	DT_CONSTANT, TK_FUNCTION, func_rev_sel)
    D("rfav",		0,           TK_INTEGER,  2)
    F("rgrep",		DT_CONSTANT, TK_FUNCTION, func_rgrep)
    F("roll",		DT_CONSTANT, TK_FUNCTION, func_roll)
    D("rsel",		DT_MULTI,    TK_STRING,   "")
    D("saverep",	DT_CONSTANT, TK_INTEGER,  0)
    F("savevar",	DT_CONSTANT, TK_FUNCTION, func_savevar)
    D("scriptdir",	DT_CONSTANT, TK_STRING,   SCRIPT_DIR)
    D("scriptname",	DT_CONSTANT, TK_STRING,   "")
    F("search",		DT_CONSTANT, TK_FUNCTION, func_search)
    D("secret",		DT_PROTECT,  TK_INTEGER,  0)
    D("secure",		DT_CONSTANT, TK_INTEGER,  1)
    F("seekuser",	DT_CONSTANT, TK_FUNCTION, func_seekuser)
    F("select",		DT_CONSTANT, TK_FUNCTION, func_select)
    F("selectuser",	DT_CONSTANT, TK_FUNCTION, func_selectuser)
    F("serverurl",	DT_CONSTANT, TK_FUNCTION, func_serverurl)
    D("sessions",	DT_CONSTANT, TK_INTEGER,  SESSIONS)
    F("setcookie",	DT_CONSTANT, TK_FUNCTION, func_setcookie)
    F("set_conf_note",	DT_CONSTANT, TK_FUNCTION, func_set_conf_note)
    F("set_item_note",	DT_CONSTANT, TK_FUNCTION, func_set_item_note)
    F("sgrep",		DT_CONSTANT, TK_FUNCTION, func_sgrep)
    D("showforgotten",	0,	     TK_INTEGER,  0)
    F("showopt",	DT_CONSTANT, TK_FUNCTION, func_showopt)
    D("since",		0,	     TK_TIME,     0)
    F("sleep",		DT_CONSTANT, TK_FUNCTION, func_sleep)
    F("snoop_conf",	DT_CONSTANT, TK_FUNCTION, func_snoop_conf)
    D("spell_lang",	0,	     TK_STRING,   "")
    D("spell_ignore",	0,	     TK_STRING,   "")
    F("spellcheck",	DT_CONSTANT, TK_FUNCTION, func_spellcheck)
    F("split",		DT_CONSTANT, TK_FUNCTION, func_split)
    F("srand",		DT_CONSTANT, TK_FUNCTION, func_srand)
    D("stack_limit",	0,	     TK_INTEGER,  0)
    F("stop",		DT_CONSTANT, TK_FUNCTION, func_stop)
    F("stopped",	DT_CONSTANT, TK_FUNCTION, func_stopped)
    F("store",		DT_CONSTANT, TK_FUNCTION, func_store)
    F("substr",		DT_CONSTANT, TK_FUNCTION, func_substr)
    F("tail",		DT_CONSTANT, TK_FUNCTION, func_tail)
    D("text",		0,	     TK_STRING,   "")
    D("texttype",	0,	     TK_STRING,   "")
    F("this_item",	DT_CONSTANT, TK_FUNCTION, func_this_item)
    F("time",		DT_CONSTANT, TK_FUNCTION, func_time)
    F("timeout",	DT_CONSTANT, TK_FUNCTION, func_timeout)
    D("timezone",	0,	     TK_STRING,   "")
    D("title",		0,	     TK_STRING,   "")
    D("uid",		DT_PROTECT,  TK_INTEGER,  0)
    F("undef",		DT_CONSTANT, TK_FUNCTION, func_undef)
    F("undefconstant",	DT_CONSTANT, TK_FUNCTION, func_undefconstant)
    F("unhtml",		DT_CONSTANT, TK_FUNCTION, func_unhtml)
    D("urlarg",		0,           TK_STRING,   "")
    F("useremail",	DT_CONSTANT, TK_FUNCTION, func_useremail)
    F("userinfo",	DT_CONSTANT, TK_FUNCTION, func_userinfo)
    D("userlist",	DT_PROTECT,  TK_INTEGER,  0)
    F("validate",	DT_CONSTANT, TK_FUNCTION, func_validate)
    F("version",	DT_CONSTANT, TK_FUNCTION, func_version)
    F("while",		DT_CONSTANT, TK_FUNCTION, func_while)
    F("wrap",		DT_CONSTANT, TK_FUNCTION, func_wrap)
    F("writable",	DT_CONSTANT, TK_FUNCTION, func_writable)
    F("write",		DT_CONSTANT, TK_FUNCTION, func_write)
    F("xdef",		DT_CONSTANT, TK_FUNCTION, func_xdef)
    F("xdefconstant",	DT_CONSTANT, TK_FUNCTION, func_xdefconstant)
    F("xstore",		DT_CONSTANT, TK_FUNCTION, func_xstore)
    D("yapp_format",	DT_CONSTANT, TK_INTEGER,  0)
    F("ztime",		DT_CONSTANT, TK_FUNCTION, func_ztime)
    F("~",		DT_CONSTANT, TK_FUNCTION, func_bnot)
    /* End of array marker */
    D("",		0,	     0,		  NULL) };


void die(const char *fmt,...)
{
va_list ap;

    va_start(ap,fmt);
    fprintf(stderr,"Error: ");
    vfprintf(stderr, fmt, ap);
    fputs("\n",stderr);
    va_end(ap);
    exit(1);
}

char *ucname(char *n)
{

    if (isalpha(n[0]) || n[0] == '_' || n[0] == '@')
    {
	static char uc[MAXSYMLEN+4];
	int i= 0, j= 0;

	/* Syntactically illegal symbol names starting an @ are for
	 * internal use - not currently used */
	if (n[0] == '@') {
		if (n[1] == '\0') return "AT";
		strcpy(uc,"INT_");
		i= 1;
		j= 4;
	}

	for ( ; n[i] != '\0'; i++,j++)
	    uc[j]= (islower(n[i]) ? toupper(n[i]) : n[i]);
	uc[j]= '\0';
	return uc;
    }

    switch(n[0])
    {
    case '!': return "NOT";
    case '~': return "BNOT";
    case '?': return "BISEL";
    case '*': return "MUL";
    case '+': return "ADD";
    case '-': return "SUB";
    case '/': return "DIV";
    case '#': return "POUND";
    case '&': return "AMPERSAND";
    case '|': return "BAR";
    case '[': return "LBRACKET";
    case ']': return "RBRACKET";
    case '=': return "EQUALS";
    case '`': return "LPRIME";
    case '\'': return "RPRIME";
    default: return NULL;
    }
}

main(int argc, char **argv)
{
    long i,j;
    int new;
    HashTable h;
    HashEntry *e;
    HashSearch s;
    char *cfile, *hfile;
    char *uc;
    FILE *cfp, *hfp;

    if (argc != 3)
    {
    	fprintf(stderr,"usage: %s <c-file> <h-file>\n",argv[0]);
	exit(1);
    }
    cfile= argv[1];
    hfile= argv[2];

    if ((cfp= fopen(cfile,"w")) == NULL)
    	die("cannot write to c-file '%s'",cfile);

    if ((hfp= fopen(hfile,"w")) == NULL)
    	die("cannot write to c-file '%s'",cfile);

    InitHashTable(&h);
    for (i= 0; dict[i].n[0] != '\0'; i++)
    {
    	e= CreateHashEntry(&h, dict[i].n, &new);
	GetHashTok(e).val= (void *)i;
    }

    fprintf(cfp,
"/* DO NOT EDIT THIS FILE\n"
" * It is automatically generated by mksysdict.c.\n"
" */\n"
"\n"
"/* SYSTEM DICTIONARY - This is the table that stores all backtalk built-in\n"
" * functions and variables.\n"
" *\n"
" * If NO_FUNCS is defined at compile time, we compile a version of the\n"
" * system dictionary that doesn't include the pointers to the functions.\n"
" */\n"
"\n"
"#include \"backtalk.h\"\n"
"#include \"sysdict.h\"\n"
"\n"
"#include <stdio.h>\n"
"\n"
"#include \"array.h\"\n"
"#include \"getput.h\"\n"
"#include \"userfunc.h\"\n"
"#include \"spell.h\"\n"
"#include \"session.h\"\n"
"\n"
"#define D(n,e,b,f,t,i) {e, &syshash, (HashEntry **)b, {t, 0, (void *)i}, f, n},\n"
"\n"
"#ifdef NO_FUNCS\n"
"\n"
"#define F(n,e,b,f,t,i) {e, &syshash, (HashEntry **)b, {t, 0, (void *)NULL}, f, n},\n"
"#define E(n,e,b,f,t,i) {e, &syshash, (HashEntry **)b, {TK_FUNCTION, 0, (void *)NULL}, f, n},\n"
"\n"
"#else\n"
"\n"
"#define F(n,e,b,f,t,i) {e, &syshash, (HashEntry **)b, {t, 0, (void *)i}, f, n},\n"
"\n"
"#ifdef DYNALOAD\n"
"#include \"dynaload.h\"\n"
"#ifdef NEED_USCORE\n"
"#define E(n,e,b,f,t,i) {e, &syshash, (HashEntry **)b, {TK_DYNAMIC, l, (void *)\"_\"#i}, f, n},\n"
"#else\n"
"#define E(n,e,b,f,t,i) {e, &syshash, (HashEntry **)b, {TK_DYNAMIC, l, (void *)#i}, f, n},\n"
"#endif\n"
"#else\n"
"#define E(n,e,b,f,t,i) {e, &syshash, (HashEntry **)b, {TK_FUNCTION, 0, (void *)i}, f, n},\n"
"#endif\n"
"\n"
"#include \"builtin.h\"\n"
"#include \"conf.h\"\n"
"#include \"dict.h\"\n"
"#include \"exec.h\"\n"
"#include \"file.h\"\n"
"#include \"format.h\"\n"
"#include \"math.h\"\n"
"#include \"stack.h\"\n"
"#include \"sel.h\"\n"
"#include \"search.h\"\n"
"#include \"regular.h\"\n"
"#include \"browser.h\"\n"
"#include \"http.h\"\n"
"#include \"ps_ulist.h\"\n"
"#include \"ps_cflist.h\"\n"
"#include \"mod_conf.h\"\n"
"#include \"adm_user.h\"\n"
"#include \"baaf.h\"\n"
"#include \"mimetype.h\"\n"
"#include \"emailfunc.h\"\n"
"#endif /* NO_FUNCS */\n\n"
   );

    fprintf(hfp,
"/* DO NOT EDIT THIS FILE\n"
" * It is automatically generated by mksysdict.c.\n"
" */\n"
"\n"
   );

    fprintf(cfp,"HashTable syshash= {\n");
    fprintf(cfp,"  NULL, {NULL,NULL,NULL,NULL}, %d, %d, %d, %d, %d, 1, 1};\n\n",
    	h.numBuckets, h.numEntries, h.rebuildSize, h.downShift, h.mask );

    fprintf(cfp,"HashEntry sysdict[]= {\n");

    for (e= FirstHashEntry(&h,&s), i= 0; e != NULL; e= NextHashEntry(&s), i++)
    {
    	e->t.line= i;
    }

    for (e= FirstHashEntry(&h,&s), i= 0; e != NULL; e= NextHashEntry(&s), i++)
    {
	j= (long)e->t.val;
    	fprintf(cfp,"    %c(\"%s\", ", dict[j].v[0], e->key);
	if (e->nextPtr == NULL)
	    fprintf(cfp,"NULL");
	else
	    fprintf(cfp,"sysdict+%d", e->nextPtr->t.line);
	fprintf(cfp,", %d, %s)\n", e->bucketPtr - h.buckets, dict[j].v+1);
	if ((uc= ucname(dict[j].n)) != NULL)
	    fprintf(hfp,"#define %s_%s %d\n",
	           (dict[j].v[0]=='D') ? "VAR" : "FUNC", uc, i);
    }
    fprintf(cfp,"};\n\n");
    fprintf(hfp,"\n#define SYSDICTLEN %d\n",i);

    fprintf(cfp,"HashEntry *sysbuc[%d]= {\n", h.numBuckets);
    for (i= 0; i < h.numBuckets; i++)
    {
    	if (h.buckets[i] == NULL)
	    fprintf(cfp,"    NULL,\n");
	else
	    fprintf(cfp,"    sysdict+%d,\n",h.buckets[i]->t.line);
    }
    fprintf(cfp,"};\n");

    fprintf(hfp,"\nvoid init_sysdict(void);\n");

    fprintf(cfp,
"\n\n"
"/* INIT_SYSDICT - Function to fix-up some missing pointers in the above.\n"
" */\n"
"\n"
"void init_sysdict()\n"
"{\n"
"    int i;\n"
"    syshash.buckets= sysbuc;\n"
"    for (i= 0; i < SYSDICTLEN; i++)\n"
"	sysdict[i].bucketPtr= sysbuc + (long)sysdict[i].bucketPtr;\n"
"}\n"
     );

#ifdef CLEANUP
    DeleteHashTable(&h);
#endif

    fclose(cfp);
    fclose(hfp);

    exit(0);
}

/* Dummy this out */
void regfree(regex_t * r) {}
