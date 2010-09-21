#ifndef DMZ_V8_QT_TEXT_EDIT_DOT_H
#define DMZ_V8_QT_TEXT_EDIT_DOT_H

#include <dmzV8QtObject.h>

class QTextEdit;

namespace dmz {

   class V8QtTextEdit : public V8QtObject {

      Q_OBJECT

      public:
         V8QtTextEdit (
            const V8Object &Self,
            QWidget *widget,
            JsModuleUiV8QtBasicState *state);

         virtual ~V8QtTextEdit ();

         virtual Boolean bind (QWidget *sender, const String &Signal);

      public Q_SLOTS:
         void on_textChanged ();
   };
};

#endif

