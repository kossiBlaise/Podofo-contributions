#include "pdfPageLabel.h"


/**************************************************************
 * Helper functions
 */


static void stringToLower(std::string &str)
{
   std::locale loc;
   for (auto &c : str) {      // convert to lower case
      c = std::tolower(c, loc);
   }
}


// conversion functions

static void cvNumToRoman(int num, std::string &numStr)
{
   numStr.clear();
   static const int values[] =   {1000, 900, 500,  400, 100,   90, 50,   40,   10,   9,    5,   4,   1};
   static const char *symbols[] = {"M", "CM", "D", "CD", "C", "XC", "L", "XL", "X", "IX", "V", "IV", "I"};
   if (num == 0) {
      numStr = "0"; // the concept of 0 did not exist in the Roman numeric alphabet
      return;
   }

   for (int i = 0; i < 13; i++) {
      while (num >= values[i]) {
         numStr.append(symbols[i]);
         num -= values[i];
      }
   }
}


static void cvFunc_D(std::string &fmtString, [[maybe_unused]] LabelStruct *lbStruct, int pageIndex, int offset)
{
   if (pageIndex < 0)
     pageIndex = -pageIndex;

   pageIndex ++; // because pageNo are indexes. Index starts with 0 which corresponds to human page 1.
   pageIndex -= offset;

   char s[50];  // one million  + page numbers
   sprintf_s(s, sizeof(s), "%d", pageIndex);
   fmtString = s;
}

// convert pageIndex to Roman uppercase

static void cvFunc_R(std::string &fmtString, [[maybe_unused]] LabelStruct *lbStruct, int pageIndex, int offset)
{
   if (pageIndex < 0)
     pageIndex = - pageIndex;

   pageIndex ++; // because pageNo are indexes. Index starts with 0 which corresponds to human page 1.
   pageIndex -= offset;

   cvNumToRoman(pageIndex, fmtString);
}

// convert pageNo to roman lower case

static void cvFunc_r(std::string &fmtString, [[maybe_unused]] LabelStruct *lbStruct, int pageIndex, int offset)
{
   cvFunc_R(fmtString, lbStruct, pageIndex, offset);
   stringToLower(fmtString);
}


static void cvFunc_A(std::string &fmtString, LabelStruct *lbStruct, int pageIndex, int offset)
{
   fmtString.clear();
   if (pageIndex < 0)
     pageIndex = - pageIndex;
   pageIndex ++; // because pageNo are indexes. Index starts with 0 which corresponds to human page 1.
   pageIndex -= offset;
   if (lbStruct->labelPrefixStart > 0) {
      pageIndex += lbStruct->labelPrefixStart -1;
   }
   char s[50];
   sprintf_s(s, sizeof(s), "%s%d", lbStruct->labelPrefix.c_str(), pageIndex);
   fmtString = s;
}


static void cvFunc_a(std::string &fmtString, LabelStruct *lbStruct, int pageIndex, int offset)
{
   cvFunc_A(fmtString, lbStruct, pageIndex, offset);
   stringToLower(fmtString);
}

// Untility functions

static void trimString(std::string &s)
{
    // Trim leading whitespace
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch)
    {
        return !std::isspace(ch);
    }));

    // Trim trailing whitespace
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch)
    {
        return !std::isspace(ch);
    }).base(), s.end());
}


static void removeStringChars(std::string &str, const std::string & charsToRemove)
{
   int index = 0;

   while(true) {
      index = str.find_first_of(charsToRemove, index);
      if (index == std::string::npos)
         break;
      str.erase(index, 1);
   }
}



// map of function converters based on the style as key
using CHAR_KEY = char;
using LabelStyleMap = std::map<const CHAR_KEY, void (*)(std::string &fmtString, LabelStruct *, int, int)>;

static LabelStyleMap labelStyleMap =      // this does not change.
{
   {'D', cvFunc_D}, {'R', cvFunc_R}, {'r', cvFunc_r },
   {'A', cvFunc_A}, {'a', cvFunc_a}
};


/*************** Class Implementation ************************/

PdfPageLabel::PdfPageLabel(PdfDocument &document) : doc(document)
{

}


PdfPageLabel::~PdfPageLabel()
{

}


nullable<PdfString> PdfPageLabel::GetPageLabel(unsigned int pageIndex)
{
   // first get numbering range base.
   nullable<PdfString>labelStr = {};
   std::string labelPrefix;

   // get the first base strictly greater than the key.
   LabelMap::iterator  iter = m_PglMap.upper_bound(pageIndex);

   if (iter == m_PglMap.end()) {  // The last base greater then pageIndex does not exist
     iter = m_PglMap.end();   // get the last element of the range.
     iter --; // get the last range base.
   }
   else if (iter != m_PglMap.begin()) {
      iter --;    // the pageindex belongs to the previous range. Get its base
   }

   int pageOffset = iter->first;
   char ch = iter->second->labelStyle;
   LabelStruct *lbStruct = iter->second.get();

   {  // check to see that the style exists.
      LabelStyleMap::iterator iter;
      iter = labelStyleMap.find(ch);
      if (iter == labelStyleMap.end()) {
         return {};
      }
   }

   // call the formating functions indexed by the style letter.

   std::string str;
   (labelStyleMap[ch])(str, lbStruct, pageIndex, pageOffset);
   labelStr = str;
   return labelStr;
}


/****************  Internal functions: Private functions  **********************************/

// This is the starting process of reading the pageLabels.

bool PdfPageLabel::readPageLabels()
{
   m_PglMap.clear();

   std::string_view keyStr("PageLabels");
      PdfCatalog &catLog = doc.GetCatalog();
      PdfDictionary &catDict = catLog.GetDictionary();      // main dictionary
      PdfIndirectObjectList &indObjList = doc.GetObjects();

      PdfObject * rootObj = catDict.GetKey(keyStr);
      if (!rootObj)
      {
         cout << "PageLabels Root not found" << endl;
         return false;
      }

      if (rootObj->IsDictionary()) {
         if (!parseLabelRoot(rootObj, indObjList))
           return false;
      }
      else if (rootObj->IsReference()) {
         PdfReference ref = rootObj->GetReference();
         PdfObject *refObj = indObjList.GetObject(ref);
         if (!refObj)
         {
            cout << "Indirect PageLabel Root not found" << endl;
            return false;
         }

         if (refObj->IsDictionary())
         {
            if (!parseLabelRoot(refObj, indObjList))
               return false;
         }
         else
         {
            cout << "Indirect PageLabelRoot is not a dictionary" << endl;
         }
      }
      else
      {
         cout << "Root Kind Not Implemented" << endl;
         return false;
      }
   return true;
}


bool PdfPageLabel::parseLabelRoot(PdfObject *rootObj, [[maybe_unused]] PdfIndirectObjectList &indObjList)
{
   // must be dictionary

   std::string keyStr;

   PdfDictionary &pglDict = rootObj->GetDictionary();
   PdfDictionary::iterator iter;

   iter = pglDict.begin();
   for (; iter != pglDict.end(); iter ++) {
      keyStr = iter->first.GetString();
      removeStringChars(keyStr, "/");

      if (keyStr == "Kids")  // root or intermediate nodes
      {
         // the value must be an array
         PdfArray &arr = iter->second.GetArray();
         processLabelArray(arr);
      }
      else if (keyStr == "Nums") // Either we have Kids or Nums, not both
      {  // Either a root or leaf Node
         // the value is an array of [key1 value1 key2 value2 ...]
         // key(i) is an integer (PageIndex), value(i) is in indirect object assosiated with this bobject
         PdfArray &arr = iter->second.GetArray();
         processNumArray(arr);
      }
      // we are not processing Limits;
   }
   return true;
}



void PdfPageLabel::processNumArray(PdfArray &arr)
{
   // The format of this array : [key1 val1 key val2 ...]
   // where key(i) is the index of the page
   // val(i) is a dictionary containing the description

   std::string labelStyle;
   std::string labelPrefix;
   int labelPrefixStart;

   for (size_t index = 0; index < arr.GetSize(); index += 2) {
      double val;
      int keyNum;

      // get the index of the page
      arr.TryGetAtAs<double>(index, val);
      keyNum = (int) val;

      // findAt will convert reference object to the indirect object automatically
      PdfObject *obj = arr.FindAt(index + 1);
      if (!obj)
        continue;

      if (obj->IsDictionary())   // then this is a leaf node
      {
         int pageIndex = keyNum;
         labelPrefixStart = 0;
         labelStyle.clear();
         labelPrefix.clear();

         // now get formatting
         PdfDictionary &dict = obj->GetDictionary();

         // get the S key (Style)
         std::string_view sKey = "S";
         PdfObject *tempObj = dict.GetKey(sKey);
         if (tempObj)  // then we have the s entry, and its value is /Name
         {
            const PdfName &tempName = tempObj->GetName();
            labelStyle = tempName.GetString();
            removeStringChars(labelStyle, "/"); // it will trim it too
         }
         // now if there is a P entry for label prefix
         sKey = "P";
         tempObj = dict.GetKey(sKey);
         if (tempObj)
         {
            // the prefix text
            const PdfString &tempString = tempObj->GetString();
            labelPrefix = tempString.GetString();
            trimString(labelPrefix);
         }
         // now St key
         sKey = "St";
         tempObj = dict.GetKey(sKey);
         if (tempObj)
         {
            double val = 0;
            tempObj->TryGetReal(val);
            labelPrefixStart = (int) val;
         }
         // store the parsed data.
         storePageLabel(pageIndex, labelStyle, labelPrefix, labelPrefixStart);
         pageIndex = -1;
      }
      else if (obj->IsArray()) {
         // we have not reached the leaf yet. keyNum is a just a key not a page Index.
         PdfArray &objArr = obj->GetArray();
         processLabelArray(objArr);
      }
   }
}


void PdfPageLabel::processLabelArray(PdfArray &arr)
{
   std::string tempStr;

   for (size_t index = 0; index < arr.GetSize(); index ++) {
      PdfObject *obj = arr.FindAt(index);
      if (!obj)
        continue;
      if (obj->IsArray())
      {
         PdfArray &xarr = obj->GetArray();
         processLabelArray(xarr);
      }
      else if (obj->IsDictionary()) // we reached a leaf.
      {
         PdfDictionary &dict = obj->GetDictionary();
         PdfDictionary::iterator iter(dict.begin());
         for (; iter != dict.end(); iter ++) {
            PdfName keyName = iter->first;
            tempStr = keyName.GetString();
            removeStringChars(tempStr, "/");    // let us be safe

            if (tempStr == "Kids")  // root or intermediate nodes
            {
               // the value must be an array
               PdfArray &arr = iter->second.GetArray();
               processLabelArray(arr);
            }
            else if (tempStr == "Nums") // Either we have Kids or Nums, not both
            {
               // Either a root or leaf Node
               // the value is an array of [key1 value1 key2 value2 ...]
               // key(i) is an integer (PageIndex), value(i) is in indirect object associated with this object

               PdfArray &arr = iter->second.GetArray();
               processNumArray(arr);
            }
         }
      }
   }
}


// store one page label in the page label map.

void PdfPageLabel::storePageLabel(int pageIndex, std::string &labelStyle, std::string &labelPrefix,
                                    int labelPrefixStart)
{

   LabelMap::iterator iter = m_PglMap.upper_bound(pageIndex);

   char ch = labelStyle.c_str()[0];

   // get the first base greater than the key.

   if (iter == m_PglMap.end()) {  // then the label is not there
      std::shared_ptr<LabelStruct> labelSt;
      labelSt.reset(new LabelStruct);
      labelSt->labelStyle = ch;    // store the style character.
      labelSt->count = 1;
      labelSt->labelPrefix = labelPrefix;
      labelSt->labelPrefixStart = labelPrefixStart;
      m_PglMap[pageIndex] = labelSt;
   }
   else  {
      // iter->first > PageIndex.
      // let us see if the previous item has the style.
      if (iter != m_PglMap.begin()) {
         iter --;
         if (iter->second->labelStyle != ch) {
            // we have to insert a new item because this page index does not belong to this range
            // construct a new one
            std::shared_ptr<LabelStruct>lst;
            lst.reset(new LabelStruct);
            lst->labelStyle = ch;
            lst->count = 1;
            lst->labelPrefix = labelPrefix;
            lst->labelPrefixStart = labelPrefixStart;
            m_PglMap[pageIndex] = lst;
         }
         else // within the same range
         {
            // have the same ch. Now check to see if same prefix
            std::shared_ptr<LabelStruct>lst = iter->second;
            if (iter->second->labelPrefix == labelPrefix) {
               iter->second->count ++;    // belongs to the same range, increase its count.
            }
            else if (iter->first != pageIndex)
            {
               // label prefix is different. The range.lower must be different from pageIndex.
               // let us add another range.
               std::shared_ptr<LabelStruct>lst = std::make_shared<LabelStruct>();
               lst->labelStyle = ch;
               lst->count = 1;
               lst->labelPrefix = labelPrefix;
               lst->labelPrefixStart = labelPrefixStart;
               m_PglMap[pageIndex] = lst;
            }
         }
      }
      else {  // new lowest page index
         // create a new labelStruct
         std::shared_ptr<LabelStruct>lst = std::make_shared<LabelStruct>();
         lst->labelStyle = ch;
         lst->count = 1;
         lst->labelPrefix = labelPrefix;
         lst->labelPrefixStart = labelPrefixStart;
         m_PglMap[pageIndex] = lst;
      }
   }
}


void PdfPageLabel::dumpPageLabelMap()
{
   cout << "\n\n*********** Dump ********\n" << endl;
   LabelMap::iterator iter(m_PglMap.begin());
   for (;iter != m_PglMap.end(); iter ++ ) {
      cout << iter->first << " " << iter->second->labelStyle
           << "  Count: " << iter->second->count << endl;
      cout << "Label Prefix \"" << iter->second->labelPrefix
           << "\"  LabelPrefixStart at: " << iter->second->labelPrefixStart << endl << endl;
   }
}