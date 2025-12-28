#include <QApplication>
#include <QDebug>

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

#undef SHOW_TRACING

#ifndef PODOFO_
   #include "fast_float/fast_float.h"
#endif

using namespace PoDoFo;


#include "PdfPageLabel.h"



int main([[maybe_unused]]int argc, [[maybe_unused]]char **argv)
{
   std::string fileName;
   PdfMemDocument doc;
   std::vector<std::string> fileNames = { "Everything-curl.pdf",
                                          "PdfReference.pdf",
                                          "pdfReference1-0.pdf",
                                          "pdfReference1-6.pdf",
                                          "vmime-book.pdf"
   };


   try {

      while(1) {
         printf("1 - Load \"everything-curl.pdf\"\n");
         printf("2 - Load \"PdfRefrence.pdf\" (Problem loading this file)\n");
         printf("3 - Load \"pdfRefrence1-0.pdf\"\n");
         printf("4 - Load \"pdfRefrence1-6.pdf\"\n");
         printf("5 - Load \"vmime-book.pdf\"\n");
         printf("Press Endter to end Only to exit\n\n");
         printf("?  ");

         char s[70];
         gets_s(s, sizeof(s));
         if (!*s) {
            return 0;
         }
         size_t index = atoi(s) - 1;
         if (index < 0 || index >= fileNames.size())
         {
            printf("********** Invalid Choice ************\n\n");
            continue;
         }
         fileName = fileNames[index];
         doc.Load(fileName);
         PdfPageLabel pgl(doc);
         bool result = pgl.readPageLabels();

         cout << "Result " << result << endl;
         if (!result)
         {
            printf("\nPdfPageLabel Instantiation failed\n\n");
            continue;
         }

         pgl.dumpPageLabelMap();
         printf("\nPress Enter Only to exit Page Index Loop\n");
         while(1) {
            printf("\nInput page Index. ?  ");
            char s[20];
            gets_s(s, sizeof(s));
            if (!*s)
              break;
            nullable<PdfString>pgNum = pgl.GetPageLabel(atoi(s));
            if (pgNum.has_value())
            {
               cout << "Page Label: " << pgNum.value().GetString() << endl;
            }
         }
         printf("\n\n");
      }
   }
   catch(PdfError &pdfErr) {
      cout << pdfErr.what() << endl;
   }
   return 0;
}
