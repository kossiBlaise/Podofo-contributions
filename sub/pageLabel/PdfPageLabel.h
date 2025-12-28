#pragma once
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include <string>
#include <stdexcept>
#include <vector>
#include <string_view>
#include <algorithm>
#include <chrono>
#include <typeinfo>
#include <cctype>
#include <locale>


using namespace std;

#include "podofo/podofo.h"

using namespace PoDoFo;

/*********** Helper struct *********/

typedef struct LabelStruct {
   char labelStyle;
   int count;        // number of pages in the range.
   std::string labelPrefix;  // label profix start.
   int labelPrefixStart;        // prefix starting point.
   int prefixCount;             // usable is prefixCount (prfix::modulo(27), remainder)

   LabelStruct()
   {
      clear();
   }

   ~LabelStruct()
   {
      // cout << " LabelStruct deleted" << endl;
   }

   void clear()
   {
      labelStyle = ' ';
      count = 0;
      labelPrefix.clear();
      labelPrefixStart = 0;
      prefixCount = 0;
   }
} LabelStruct;


using LabelMap = std::map<int, std::shared_ptr<LabelStruct>>;

class /*PODOFO_API */ PdfPageLabel
{
   public:
      PdfPageLabel(PdfDocument &document);
      ~PdfPageLabel();

      /** Get the Page label associated with page Index. Note that the page Label
      *   returned may not be the same as PdfPage::GePageNumber()
      *  \param pageIndex is the page index number. The base of pageIndex is zero, and should be
      *   less then the total number of pages returned by PdfPageCollection::GetCount().
      *  \return nullable<PdfString> pageLabel
      *
      */

      nullable<PdfString> GetPageLabel(unsigned int pageIndex);

   // private:
      bool readPageLabels();
      bool parseLabelRoot();
      inline int  labelCount() { return m_PglMap.size();}
      void storePageLabel(int pageIndex, std::string &labelStyle, std::string &labelPrefix, int labelPrefixStart);
      void processNumArray(PdfArray &arr);
      void processLabelArray(PdfArray &arr);
      bool parseLabelRoot(PdfObject *rootObj, [[maybe_unused]] PdfIndirectObjectList &indObjList);
      void dumpPageLabelMap();


   private:
      LabelMap m_PglMap;
      PdfDocument &doc;

};
