// regress.cpp -- Regression test program, main code
// $Id: regress.cpp 1265 2007-03-09 16:52:32Z jcw $
// This is part of MetaKit, the homepage is http://www.equi4.com/metakit/

#include "regress.h"

#if defined (macintosh)  
  #include <SIOUX.h>
  #ifdef DEBUG_NEW
    #include <DebugNew.cp>
    //#include "ZoneRanger.c"
  #if DEBUG_NEW >= 2 && !defined (q4_MAC_LEAK_CHECK)
    #define q4_MAC_LEAK_CHECK 1
  #endif
  #endif
#endif

#if __profile__
  #define q4_MWCW_PROFILER 1
  #include <profiler.h>
#endif

#if q4_NOTHROW
  const char* msg;
#endif

#if q4_WINCE
#include <afxcview.h>
#endif

int
#if q4_WINCE
mainfunc()
#else
main()
#endif
{
//  afxMemDF |= allocMemDF | checkAlwaysMemDF;

    // The M4STRING package sometimes keeps a spare empty string around.
    // By allocating it here, we avoid a few bogus memory leak reports.
  c4_String empty;

  #if q4_MAC_LEAK_CHECK
    DebugNewForgetLeaks();
  #endif

#if q4_MWCW_PROFILER
  if (!ProfilerInit(collectDetailed, bestTimeBase, 20, 5))
  {
#endif
    TestBasics1();
    TestBasics2();
    TestNotify();
    TestCustom1();
    TestCustom2();
    TestResize();
    TestStores1();
    TestStores2();
    TestStores3();
    TestStores4();
    TestStores5();
    TestDiffer();
    TestExtend();
    TestFormat();
    TestMapped();
    TestLimits();

#if q4_MWCW_PROFILER
    ProfilerDump("\pRegress.prof");
    ProfilerTerm();
  }
#endif

  #if defined (_DEBUG)
    fputs("\nPress return... ", stderr);
    getchar();
  #endif    
  #if q4_MAC_LEAK_CHECK
    if (DebugNewReportLeaks())
      fputs("    *** Memory leaks found ***\n", stderr);
  #endif
  #if defined (macintosh)  
    SIOUXSettings.asktosaveonclose = false;
  #endif
  
  fputs("Done.\n", stderr);
  return 0;
}

// Recursively display the entire view contents. The results shown do not
// depend on file layout (free space, file positions, flat vs. on-demand).

static void ViewDisplay(const c4_View& v_, FILE* fp, int l_ =0)
{
  c4_String types;
  bool hasData = false, hasSubs = false;

    // display header info and collect all data types
  fprintf(fp, "%*s VIEW %5d rows =", l_, "", v_.GetSize());
  for (int n = 0; n < v_.NumProperties(); ++n)
  {
    c4_Property prop = v_.NthProperty(n);
    char t = prop.Type();

    fprintf(fp, " %s:%c", (const char*) prop.Name(), t);
    
    types += t;
  
    if (t == 'V')
      hasSubs = true;
    else
      hasData = true;
  }
  fprintf(fp, "\n");

  for (int j = 0; j < v_.GetSize(); ++j)
  {
    if (hasData)  // data properties are all shown on the same line
    {
      fprintf(fp, "%*s %4d:", l_, "", j);
      c4_RowRef r = v_[j];

      for (int k = 0; k < types.GetLength(); ++k)
      {
        c4_Property p = v_.NthProperty(k);

        switch (types[k])
        {
        case 'I':
          fprintf(fp, " %ld", (long) ((c4_IntProp&) p) (r));
          break;

        case 'F':
          fprintf(fp, " %g", (double) ((c4_FloatProp&) p) (r));
          break;

        case 'D':
          fprintf(fp, " %.12g", (double) ((c4_DoubleProp&) p) (r));
          break;

        case 'S':
          fprintf(fp, " '%s'", (const char*) ((c4_StringProp&) p) (r));
          break;

    case 'M': // backward compatibility
        case 'B':
          fprintf(fp, " (%db)", (p (r)).GetSize());
          break;

        default:
          if (types[k] != 'V')
            fprintf(fp, " (%c?)", types[k]);
        }
      }

      fprintf(fp, "\n");
    }

    if (hasSubs)  // subviews are then shown, each as a separate block
    {
      for (int k = 0; k < types.GetLength(); ++k)
      {
        if (types[k] == 'V')
        {
          c4_Property prop = v_.NthProperty(k);

          fprintf(fp, "%*s %4d: subview '%s'\n", l_, "", j,
              (const char*) prop.Name());

          c4_ViewProp& vp = (c4_ViewProp&) prop;

          ViewDisplay(vp (v_[j]), fp, l_ + 2);
        }
      }
    }
  }
}

void DumpFile(const char* in_, const char* out_)
{
  FILE* fp = fopen(out_, TEXTOUT);
  A(fp);

  c4_Storage store (in_, 0);

  ViewDisplay(store, fp);

  fclose(fp);
}

#if q4_WINCE
  static void ShowString(const CString& str_)
  {
    CView* v = ((CFrameWnd*) AfxGetApp()->m_pMainWnd)->GetActiveView();
    ((CListView*) v)->GetListCtrl().InsertItem(INT_MAX, (LPCTSTR) str_);
  }
#endif

void Fail(const char* msg)
{ 
  #if q4_NOTHROW
    #if q4_WINCE
      CString str;
      str.Format(_T("*** %s ***"), (LPCTSTR) CString (msg));
      ShowString(str);
      ASSERT(0);
    #else
      fprintf(stderr, "\t%s\n", msg);
      printf("*** %s ***\n", msg);
    #endif
  #else
    throw msg;
  #endif
}

void FailExpr(const char* expr)
{
  char buffer [100];
  sprintf(buffer, "Failed: A(%s)", expr);
  Fail(buffer);
}

int StartTest(int mask_, const char* name_, const char* desc_)
{
  if (mask_)
  {
    #if q4_MFC && defined(_DEBUG)
      TRACE("%s - %-40s *** DISABLED ***\n", name_, desc_);
    #endif
    #if !q4_MWCW_PROFILER
      fprintf(stderr, "%s - %-40s   *** DISABLED ***\n", name_, desc_);
    #endif
    return false;
  }
  
  #if q4_MFC && defined(_DEBUG)
    TRACE("%s - %s\n", name_, desc_);
  #endif
  #if q4_WINCE
    CString str;
    str.Format(_T("%s - %s"), (LPCTSTR) CString (name_),
				(LPCTSTR) CString (desc_));
    ShowString(str);
  #endif
  #if !q4_MWCW_PROFILER
    fprintf(stderr, "%s - %s\n", name_, desc_);
  #endif

  char buffer [50];
  sprintf(buffer, "%s%s.txt", TESTDIR, name_);
  #if q4_WINCE
    //TODO: Fix this to work under CE.
  #else
    freopen(buffer, TEXTOUT, stdout);
  #endif
  printf(">>> %s\n", desc_);

  return true;
}

void CatchMsg(const char* msg)
{
  #if !q4_MWCW_PROFILER
    fprintf(stderr, "\t%s\n", msg);
  #endif
  printf("*** %s ***\n", msg);
}

void CatchOther()
{
  #if !q4_MWCW_PROFILER
    fputs("\tException!\n", stderr);
  #endif
  printf("*** Exception ***\n");
}
