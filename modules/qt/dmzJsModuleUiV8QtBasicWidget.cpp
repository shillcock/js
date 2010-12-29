#include "dmzJsModuleUiV8QtBasic.h"
#include <dmzJsModuleV8.h>
#include <dmzJsV8UtilConvert.h>
#include "dmzV8QtUtil.h"
#include "dmzV8QtWidget.h"
#include <QtGui/QWidget>
#include <QtGui/QLayout>

#include <QtGui/QPushButton>

dmz::V8Value
dmz::JsModuleUiV8QtBasic::_widget_close (const v8::Arguments &Args) {

   v8::HandleScope scope;
   V8Value result = v8::Undefined ();

   JsModuleUiV8QtBasic *self = _to_self (Args);
   if (self) {

      QWidget *widget = self->_to_qwidget (Args.This ());
      if (widget) {

         widget->close ();
      }
   }

   return scope.Close (result);
}


dmz::V8Value
dmz::JsModuleUiV8QtBasic::_widget_enabled (const v8::Arguments &Args) {

   v8::HandleScope scope;
   V8Value result = v8::Undefined ();

   JsModuleUiV8QtBasic *self = _to_self (Args);
   if (self) {

      QWidget *widget = self->_to_qwidget (Args.This ());
      if (widget) {

         if (Args.Length ()) {

            const Boolean Value = v8_to_boolean (Args[0]);
            widget->setEnabled (Value);
         }

         result = v8::Boolean::New (widget->isEnabled ());
      }
   }

   return scope.Close (result);
}


dmz::V8Value
dmz::JsModuleUiV8QtBasic::_widget_hide (const v8::Arguments &Args) {

   v8::HandleScope scope;
   V8Value result = v8::Undefined ();

   JsModuleUiV8QtBasic *self = _to_self (Args);
   if (self) {

      QWidget *widget = self->_to_qwidget (Args.This ());
      if (widget) { widget->hide (); }
   }

   return scope.Close (result);
}


dmz::V8Value
dmz::JsModuleUiV8QtBasic::_widget_layout (const v8::Arguments &Args) {

   v8::HandleScope scope;
   V8Value result = v8::Undefined ();

   JsModuleUiV8QtBasic *self = _to_self (Args);
   if (self) {

      QWidget *widget = self->_to_qwidget (Args.This ());
      if (Args.Length () > 0) {

         QLayout *layout = self->v8_to_qobject<QLayout> (Args[0]);
         if (layout) {

            widget->setLayout (layout);
         }
      }

      result = self->create_v8_qobject (widget->layout ());
   }

   return scope.Close (result);
}


dmz::V8Value
dmz::JsModuleUiV8QtBasic::_widget_show (const v8::Arguments &Args) {

   v8::HandleScope scope;
   V8Value result = v8::Undefined ();

   JsModuleUiV8QtBasic *self = _to_self (Args);
   if (self) {

      QWidget *widget = self->_to_qwidget (Args.This ());
      if (widget) { widget->show (); }
   }

   return scope.Close (result);
}


dmz::V8Value
dmz::JsModuleUiV8QtBasic::_widget_title (const v8::Arguments &Args) {

   v8::HandleScope scope;
   V8Value result = v8::Undefined ();

   JsModuleUiV8QtBasic *self = _to_self (Args);
   if (self) {

      QWidget *widget = self->_to_qwidget (Args.This ());
      if (widget) {

         if (Args.Length ()) {

            widget->setWindowTitle (v8_to_qstring (Args[0]));
         }

         result = qstring_to_v8 (widget->windowTitle ());
      }
   }

   return scope.Close (result);
}


dmz::V8Value
dmz::JsModuleUiV8QtBasic::_widget_visible (const v8::Arguments &Args) {

   v8::HandleScope scope;
   V8Value result = v8::Undefined ();

   JsModuleUiV8QtBasic *self = _to_self (Args);
   if (self) {

      QWidget *widget = self->_to_qwidget (Args.This ());
      if (widget) {

         if (Args.Length ()) {

            widget->setVisible (v8_to_boolean (Args[0]));
         }

         result = v8::Boolean::New (widget->isVisible ());
      }
   }

   return scope.Close (result);
}


dmz::V8Value
dmz::JsModuleUiV8QtBasic::_widget_window (const v8::Arguments &Args) {

   v8::HandleScope scope;
   V8Value result = v8::Undefined ();

   JsModuleUiV8QtBasic *self = _to_self (Args);
   if (self) {

      QWidget *widget = self->_to_qwidget (Args.This ());
      if (widget) {

         QWidget *window = widget->window ();
         if (window) {

            result = self->create_v8_qwidget (window);
         }
      }
   }

   return scope.Close (result);
}


dmz::V8Value
dmz::JsModuleUiV8QtBasic::_create_widget (const v8::Arguments &Args) {

   v8::HandleScope scope;
   V8Value result = v8::Undefined ();

   JsModuleUiV8QtBasic *self = _to_self (Args);
   if (self) {

      QWidget *parent = 0;
      if (Args.Length ()) { parent = self->_to_qwidget (Args[0]); }
      QWidget *widget = new QWidget (parent);
      result = self->create_v8_qobject (widget);
   }

   return scope.Close (result);
}

void
dmz::JsModuleUiV8QtBasic::_init_widget () {

   v8::HandleScope scope;

   _widgetTemp = V8FunctionTemplatePersist::New (v8::FunctionTemplate::New ());
   _widgetTemp->Inherit (_objectTemp);

   V8ObjectTemplate instance = _widgetTemp->InstanceTemplate ();
   instance->SetInternalFieldCount (1);

   V8ObjectTemplate proto = _widgetTemp->PrototypeTemplate ();
   proto->Set ("close", v8::FunctionTemplate::New (_widget_close, _self));
   proto->Set ("enabled", v8::FunctionTemplate::New (_widget_enabled, _self));
   proto->Set ("hide", v8::FunctionTemplate::New (_widget_hide, _self));
   proto->Set ("layout", v8::FunctionTemplate::New (_widget_layout, _self));
   proto->Set ("show", v8::FunctionTemplate::New (_widget_show, _self));
   proto->Set ("title", v8::FunctionTemplate::New (_widget_title, _self));
   proto->Set ("visible", v8::FunctionTemplate::New (_widget_visible, _self));
   proto->Set ("window", v8::FunctionTemplate::New (_widget_window, _self));

   _widgetApi.add_function ("create", _create_widget, _self);
}

