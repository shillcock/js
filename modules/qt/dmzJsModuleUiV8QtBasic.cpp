#include "dmzJsModuleUiV8QtBasic.h"
#include <dmzJsModuleV8.h>
#include <dmzJsV8UtilConvert.h>
#include <dmzQtModuleMainWindow.h>
#include <dmzQtZipFileEngineHandler.h>
#include <dmzRuntimeConfig.h>
#include <dmzRuntimeConfigToStringContainer.h>
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzSystemFile.h>
#include <dmzTypesStringTokenizer.h>
#include "dmzV8QtTypes.h"
#include "dmzV8QtUtil.h"
#include <QtCore/QFile>
#include <QtCore/QEvent>
#include <QtGui/QDockWidget>
#include <QtGui/QKeyEvent>
#include <QtGui/QLineEdit>
#include <QtGui/QMainWindow>
#include <QtUiTools/QUiLoader>


dmz::V8Value
dmz::JsModuleUiV8QtBasic::_uiloader_load (const v8::Arguments &Args) {

   v8::HandleScope scope;
   V8Value result = v8::Undefined ();

   JsModuleUiV8QtBasic *self = _to_self (Args);
   const String Name = v8_to_string (Args[0]);

   if (self && Name) {

//      const String FileName = self->_find_ui_file (Name);
      const String FileName = Name;

      if (FileName) {

         String zipFile ("test.zip");
         QtZipFileEngineHandler zipHandler (zipFile, &(self->_log));

         QUiLoader loader;
         QFile file (FileName.get_buffer ());
         file.open (QFile::ReadOnly);
         QWidget *widget = loader.load (&file, 0);
         file.close ();

         if (widget) {

            widget->setProperty ("_uiloader_loaded", True);
            widget->setProperty ("_uiloader_file", file.fileName ());
            widget->setProperty ("_uiloader_name", Name.get_buffer ());
            widget->setProperty ("_uiloader_object_name", widget->objectName ());

            QList<QLineEdit *> list = widget->findChildren<QLineEdit *>();
            foreach (QLineEdit *lineEdit, list) {

               lineEdit->installEventFilter (self->_state.ui);
               lineEdit->setProperty ("_uiloader_event_filter_installed", True);
            }
         }

         result = self->create_v8_qwidget (widget);

         if (!result.IsEmpty ()) {

            self->_log.info << "Loaded UI: " << FileName << endl;
         }
         else {

            self->_log.error << "Failed to load UI file: " << FileName << endl;
         }
      }
      else { self->_log.error << "Failed to find UI file: " << Name << endl; }
   }

   return scope.Close (result);
}


dmz::JsModuleUiV8QtBasic::JsModuleUiV8QtBasic (const PluginInfo &Info, Config &local) :
      QObject (0),
      Plugin (Info),
      TimeSlice (Info),
      JsModuleUiV8Qt (Info),
      JsExtV8 (Info),
      _log (Info),
      _centralWidget (0) {

   _state.ui = this;
   _init (local);
}


dmz::JsModuleUiV8QtBasic::~JsModuleUiV8QtBasic () {;}


// Plugin Interface
void
dmz::JsModuleUiV8QtBasic::update_plugin_state (
      const PluginStateEnum State,
      const UInt32 Level) {

   if (State == PluginStateInit) {

   }
   else if (State == PluginStateStart) {

   }
   else if (State == PluginStateStop) {

   }
   else if (State == PluginStateShutdown) {

   }
}


void
dmz::JsModuleUiV8QtBasic::discover_plugin (
      const PluginDiscoverEnum Mode,
      const Plugin *PluginPtr) {

   if (Mode == PluginDiscoverAdd) {

      if (!_state.mainWindowModule) {

         _state.mainWindowModule =
            QtModuleMainWindow::cast (PluginPtr, _mainWindowModuleName);
      }
   }
   else if (Mode == PluginDiscoverRemove) {

      if (_state.mainWindowModule &&
          (_state.mainWindowModule == QtModuleMainWindow::cast (PluginPtr))) {

         _state.mainWindowModule = 0;
      }
   }
}


// TimeSlice Interface
void
dmz::JsModuleUiV8QtBasic::update_time_slice (const Float64 DeltaTime) {

   if (!_valueDeleteList.isEmpty ()) {

      v8::Context::Scope cscope (_state.context);
      v8::HandleScope scope;

      while (!_valueDeleteList.isEmpty ()) {

         V8ValueRef *value = _valueDeleteList.takeFirst ();
         delete value; value = 0;
      }
   }
}


// JsModuleUiV8Qt Interface
dmz::V8Value
dmz::JsModuleUiV8QtBasic::create_v8_qobject (QObject *value) {

   v8::Context::Scope cscope (_state.context);
   v8::HandleScope scope;

   V8Value result = v8::Undefined ();

   if (value) {

      V8Object vobj;
      V8QtObject *qobj = _objectMap[value];

      if (!qobj) {

         if (value->isWidgetType ()) {

            QWidget  *widget = qobject_cast<QWidget *>(value);
            if (widget) { result = create_v8_qwidget (widget); }
         }
         else if (value->inherits ("QAction")) {

            if (!_actionCtor.IsEmpty ()) {

               vobj = _actionCtor->NewInstance ();
               qobj = new V8QtAction (vobj, value, &_state);
            }
         }
         else if (value->inherits ("QHBoxLayout")) {

            if (!_hBoxLayoutCtor.IsEmpty ()) {

               vobj = _hBoxLayoutCtor->NewInstance ();
               qobj = new V8QtObject (vobj, value, &_state);
            }
         }
         else if (value->inherits ("QVBoxLayout")) {

            if (!_vBoxLayoutCtor.IsEmpty ()) {

               vobj = _vBoxLayoutCtor->NewInstance ();
               qobj = new V8QtObject (vobj, value, &_state);
            }
         }
         else if (value->inherits ("QBoxLayout")) {

            if (!_boxLayoutCtor.IsEmpty ()) {

               vobj = _boxLayoutCtor->NewInstance ();
               qobj = new V8QtObject (vobj, value, &_state);
            }
         }
         else if (value->inherits ("QGridLayout")) {

            if (!_gridLayoutCtor.IsEmpty ()) {

               vobj = _gridLayoutCtor->NewInstance ();
               qobj = new V8QtObject (vobj, value, &_state);
            }
         }
         else if (value->inherits ("QFormLayout")) {

            if (!_formLayoutCtor.IsEmpty ()) {

               vobj = _formLayoutCtor->NewInstance ();
               qobj = new V8QtObject (vobj, value, &_state);
            }
         }
         else if (value->inherits ("QLayout")) {

            if (!_layoutCtor.IsEmpty ()) {

               vobj = _layoutCtor->NewInstance ();
               qobj = new V8QtObject (vobj, value, &_state);
            }
         }

         if (qobj) { _objectMap.insert (value, qobj); }
      }

      if (qobj && !(qobj->self.IsEmpty ())) { result = qobj->self; }
   }

   return scope.Close (result);
}


dmz::V8Value
dmz::JsModuleUiV8QtBasic::create_v8_qwidget (QWidget *value) {

   v8::Context::Scope cscope (_state.context);
   v8::HandleScope scope;

   V8Value result = v8::Undefined ();

   if (value) {

      V8Object vobj;
      V8QtObject *qobj = _objectMap[value];

      if (!qobj) {

         if (value->inherits ("QTreeWidget")) {

            if (!_treeWidgetCtor.IsEmpty ()) {

               vobj = _treeWidgetCtor->NewInstance ();
               qobj = new V8QtTreeWidget (vobj, value, &_state);
            }
         }
         else if (value->inherits ("QInputDialog")) {

            if (!_inputDialogCtor.IsEmpty ()) {

               vobj = _inputDialogCtor->NewInstance ();
               qobj = new V8QtInputDialog (vobj, value, &_state);
            }
         }
         else if (value->inherits ("QMessageBox")) {

            if (!_messageboxCtor.IsEmpty ()) {

               vobj = _messageboxCtor->NewInstance();
               qobj = new V8QtMessageBox (vobj, value, &_state);

               _dialogList.append (value);
            }
         }
         else if (value->inherits ("QToolBox")) {

            if (!_toolboxWidgetCtor.IsEmpty ()) {

               vobj = _toolboxWidgetCtor->NewInstance ();
               qobj = new V8QtToolBox (vobj, value, &_state);
            }
         }
         else if (value->inherits ("QTableWidget")) {

            if (!_tableWidgetCtor.IsEmpty ()) {

               vobj = _tableWidgetCtor->NewInstance ();
               qobj = new V8QtTableWidget (vobj, value, &_state);
            }
         }
         else if (value->inherits ("QTabWidget")) {

            if (!_tabCtor.IsEmpty ()) {

               vobj = _tabCtor->NewInstance ();
               qobj = new V8QtTabWidget (vobj, value, &_state);
            }
         }
         else if (value->inherits ("QDateTimeEdit")) {

            if (!_dteCtor.IsEmpty()) {

               vobj = _dteCtor->NewInstance ();
               qobj = new V8QtDateTimeEdit (vobj, value, &_state);
            }
         }
         else if (value->inherits ("QGroupBox")) {

            if (!_groupBoxCtor.IsEmpty ()) {

               vobj = _groupBoxCtor->NewInstance ();
               qobj = new V8QtGroupBox (vobj, value, &_state);
            }
         }
         else if (value->inherits ("QStackedWidget")) {

            if (!_stackedCtor.IsEmpty ()) {

               vobj = _stackedCtor->NewInstance ();
               qobj = new V8QtStackedWidget (vobj, value, &_state);
            }
         }
         else if (value->inherits ("QDialog")) {

            if (!_dialogCtor.IsEmpty ()) {

               vobj = _dialogCtor->NewInstance ();
               qobj = new V8QtDialog (vobj, value, &_state);

               _dialogList.append (value);
            }
         }
         else if (value->inherits ("QProgressBar")) {

            if (!_progressBarCtor.IsEmpty ()) {

               vobj = _progressBarCtor->NewInstance ();
               qobj = new V8QtProgressBar (vobj, value, &_state);
            }
         }
         else if (value->inherits ("QTextEdit")) {

            if (!_textEditCtor.IsEmpty ()) {

               vobj = _textEditCtor->NewInstance ();
               qobj = new V8QtTextEdit (vobj, value, &_state);
            }
         }
         else if (value->inherits ("QLineEdit")) {

            if (!_lineEditCtor.IsEmpty ()) {

               vobj = _lineEditCtor->NewInstance ();
               qobj = new V8QtLineEdit (vobj, value, &_state);
            }
         }
         else if (value->inherits ("QComboBox")) {

            if (!_comboBoxCtor.IsEmpty ()) {

               vobj = _comboBoxCtor->NewInstance ();
               qobj = new V8QtComboBox (vobj, value, &_state);
            }
         }
         else if (value->inherits ("QSlider")) {

            if (!_sliderCtor.IsEmpty ()) {

               vobj = _sliderCtor->NewInstance ();
               qobj = new V8QtSlider (vobj, value, &_state);
            }
         }
         else if (value->inherits ("QDial")) {

            if (!_dialCtor.IsEmpty ()) {

               vobj = _dialCtor->NewInstance ();

               // QDial has same signals as QSlider -ss
               qobj = new V8QtSlider (vobj, value, &_state);
            }
         }
         else if (value->inherits ("QAbstractButton")) {

            if (!_buttonCtor.IsEmpty ()) {

               vobj = _buttonCtor->NewInstance ();
               qobj = new V8QtButton (vobj, value, &_state);
            }
         }
         else if (value->inherits ("QAbstractSpinBox")) {

            if (!_spinBoxCtor.IsEmpty ()) {

               vobj = _spinBoxCtor->NewInstance ();
               qobj = new V8QtSpinBox (vobj, value, &_state);
            }
         }
         else if (value->inherits ("QListWidget")) {

            if (!_listWidgetCtor.IsEmpty ()) {

               vobj = _listWidgetCtor->NewInstance ();
               qobj = new V8QtListWidget (vobj, value, &_state);
            }
         }
         else if (value->inherits ("QDockWidget")) {

            if (!_dockWidgetCtor.IsEmpty ()) {

               vobj = _dockWidgetCtor->NewInstance ();
               qobj = new V8QtDockWidget (vobj, value, &_state);
            }
         }
         else if (value->inherits ("QLabel")) {

            if (!_labelCtor.IsEmpty ()) {

               vobj = _labelCtor->NewInstance ();

               // QLabel has no signals so it can be wrapped in a V8QtWidget -ss
               qobj = new V8QtWidget (vobj, value, &_state);
            }
         }
         else if (value->inherits ("QLCDNumber")) {

            if (!_lcdNumberCtor.IsEmpty ()) {

               vobj = _lcdNumberCtor->NewInstance ();

               // QLCDNumber has no signals so it can be wrapped in a V8QtWidget -ss
               qobj = new V8QtWidget (vobj, value, &_state);
            }
         }
         else if (value->inherits ("QFrame")) {

            if (!_frameCtor.IsEmpty ()) {

               vobj = _frameCtor->NewInstance ();
               qobj = new V8QtWidget (vobj, value, &_state);
            }
         }
         else if (value->inherits ("QWidget")) {

            if (!_widgetCtor.IsEmpty ()) {

               vobj = _widgetCtor->NewInstance ();
               qobj = new V8QtWidget (vobj, value, &_state);
            }
         }

         if (qobj) { _objectMap.insert (value, qobj); }
      }

      if (qobj && !(qobj->self.IsEmpty ())) { result = qobj->self; }
   }

   return scope.Close (result);
}


// JsExtV8 Interface
void
dmz::JsModuleUiV8QtBasic::update_js_module_v8 (
const ModeEnum Mode, JsModuleV8 &module) {

   if (Mode == JsExtV8::Store) {

      if (!_state.core) { _state.core = &module; }
   }
   else if (Mode == JsExtV8::Remove) {

      if (_state.core == &module) { _state.core = 0; }
   }
}


void
dmz::JsModuleUiV8QtBasic::update_js_context_v8 (v8::Handle<v8::Context> context) {

   _state.context = context;
}


void
dmz::JsModuleUiV8QtBasic::update_js_ext_v8_state (const StateEnum State) {

   v8::Context::Scope cscope (_state.context);
   v8::HandleScope scope;

   if (State == JsExtV8::Register) {

      if (!_objectTemp.IsEmpty ()) {

         _objectCtor = V8FunctionPersist::New (_objectTemp->GetFunction ());
      }

      if (!_treeWidgetTemp.IsEmpty ()) {

         _treeWidgetCtor = V8FunctionPersist::New (_treeWidgetTemp->GetFunction ());
      }

      if (!_toolboxTemp.IsEmpty ()) {

         _toolboxWidgetCtor = V8FunctionPersist::New (_toolboxTemp->GetFunction ());
      }

      if (!_treeWidgetItemTemp.IsEmpty ()) {

         _treeWidgetItemCtor = V8FunctionPersist::New (_treeWidgetItemTemp->GetFunction ());
      }

      if (!_tableWidgetTemp.IsEmpty ()) {

         _tableWidgetCtor = V8FunctionPersist::New (_tableWidgetTemp->GetFunction ());
      }

      if (!_tableWidgetItemTemp.IsEmpty ()) {

         _tableWidgetItemCtor = V8FunctionPersist::New (_tableWidgetItemTemp->GetFunction ());
      }

      if (!_tableSelectionTemp.IsEmpty ()) {

         _tableSelectionCtor = V8FunctionPersist::New (_tableSelectionTemp->GetFunction ());
      }

      if (!_widgetTemp.IsEmpty ()) {

         _widgetCtor = V8FunctionPersist::New (_widgetTemp->GetFunction ());
      }

      if (!_groupBoxTemp.IsEmpty ()) {

         _groupBoxCtor = V8FunctionPersist::New (_groupBoxTemp->GetFunction ());
      }

      if (!_frameTemp.IsEmpty ()) {

         _frameCtor = V8FunctionPersist::New (_frameTemp->GetFunction ());
      }

      if (!_buttonTemp.IsEmpty ()) {

         _buttonCtor = V8FunctionPersist::New (_buttonTemp->GetFunction ());
      }

      if (!_listWidgetItemTemp.IsEmpty ()) {

         _listWidgetItemCtor =
            V8FunctionPersist::New (_listWidgetItemTemp->GetFunction ());
      }

      if (!_listWidgetTemp.IsEmpty ()) {

         _listWidgetCtor = V8FunctionPersist::New (_listWidgetTemp->GetFunction ());
      }

      if (!_spinBoxTemp.IsEmpty ()) {

         _spinBoxCtor = V8FunctionPersist::New (_spinBoxTemp->GetFunction ());
      }

      if (!_comboBoxTemp.IsEmpty ()) {

         _comboBoxCtor = V8FunctionPersist::New (_comboBoxTemp->GetFunction ());
      }

      if (!_sliderTemp.IsEmpty ()) {

         _sliderCtor = V8FunctionPersist::New (_sliderTemp->GetFunction ());
      }

      if (!_dialTemp.IsEmpty ()) {

         _dialCtor = V8FunctionPersist::New (_dialTemp->GetFunction ());
      }

      if (!_lineEditTemp.IsEmpty ()) {

         _lineEditCtor = V8FunctionPersist::New (_lineEditTemp->GetFunction ());
      }

      if (!_textEditTemp.IsEmpty ()) {

         _textEditCtor = V8FunctionPersist::New (_textEditTemp->GetFunction ());
      }

      if (!_labelTemp.IsEmpty ()) {

         _labelCtor = V8FunctionPersist::New (_labelTemp->GetFunction ());
      }

      if (!_progressBarTemp.IsEmpty ()) {

         _progressBarCtor = V8FunctionPersist::New (_progressBarTemp->GetFunction ());
      }

      if (!_dialogTemp.IsEmpty ()) {

         _dialogCtor = V8FunctionPersist::New (_dialogTemp->GetFunction ());
      }

      if (!_lcdNumberTemp.IsEmpty ()) {

         _lcdNumberCtor = V8FunctionPersist::New (_lcdNumberTemp->GetFunction ());
      }

      if (!_stackedWidgetTemp.IsEmpty ()) {

         _stackedCtor = V8FunctionPersist::New (_stackedWidgetTemp->GetFunction ());
      }

      if (!_tabWidgetTemp.IsEmpty ()) {

         _tabCtor = V8FunctionPersist::New (_tabWidgetTemp->GetFunction ());
      }

      if (!_layoutTemp.IsEmpty ()) {

         _layoutCtor = V8FunctionPersist::New (_layoutTemp->GetFunction ());
      }

      if (!_boxLayoutTemp.IsEmpty ()) {

         _boxLayoutCtor = V8FunctionPersist::New (_boxLayoutTemp->GetFunction ());
      }

      if (!_hBoxLayoutTemp.IsEmpty ()) {

         _hBoxLayoutCtor = V8FunctionPersist::New (_hBoxLayoutTemp->GetFunction ());
      }

      if (!_vBoxLayoutTemp.IsEmpty ()) {

         _vBoxLayoutCtor = V8FunctionPersist::New (_vBoxLayoutTemp->GetFunction ());
      }

      if (!_gridLayoutTemp.IsEmpty ()) {

         _gridLayoutCtor = V8FunctionPersist::New (_gridLayoutTemp->GetFunction ());
      }

      if (!_formLayoutTemp.IsEmpty ()) {

         _formLayoutCtor = V8FunctionPersist::New (_formLayoutTemp->GetFunction ());
      }

      if (!_mainWindowTemp.IsEmpty ()) {

         _mainWindowCtor = V8FunctionPersist::New (_mainWindowTemp->GetFunction ());
      }

      if (!_dockWidgetTemp.IsEmpty ()) {

         _dockWidgetCtor = V8FunctionPersist::New (_dockWidgetTemp->GetFunction ());
      }

      if (!_dteTemp.IsEmpty ()) {

         _dteCtor = V8FunctionPersist::New (_dteTemp->GetFunction ());
      }

      if (!_actionTemp.IsEmpty ()) {

         _actionCtor = V8FunctionPersist::New (_actionTemp->GetFunction ());
      }

      if (!_messageboxTemp.IsEmpty ()) {

         _messageboxCtor = V8FunctionPersist::New (_messageboxTemp->GetFunction ());
      }

      if (!_inputDialogTemp.IsEmpty ()) {

         _inputDialogCtor = V8FunctionPersist::New (_inputDialogTemp->GetFunction ());
      }

      if (_state.core) {

         _state.core->register_interface ("dmz/ui/consts", _qtApi.get_new_instance ());

         _state.core->register_interface ("dmz/ui/frame", _frameApi.get_new_instance ());
         _state.core->register_interface ("dmz/ui/groupBox", _groupBoxApi.get_new_instance ());

         _state.core->register_interface (
            "dmz/ui/uiLoader",
            _uiLoaderApi.get_new_instance ());

         _state.core->register_interface (
            "dmz/ui/mainWindow",
            _mainWindowApi.get_new_instance ());

         _state.core->register_interface (
            "dmz/ui/dockWidget",
            _dockWidgetApi.get_new_instance ());

         _state.core->register_interface (
            "dmz/ui/messageBox",
            _messageBoxApi.get_new_instance ());

         _state.core->register_interface (
            "dmz/ui/layout",
            _layoutApi.get_new_instance ());

         _state.core->register_interface (
            "dmz/ui/fileDialog",
            _fileDialogApi.get_new_instance ());

         _state.core->register_interface (
            "dmz/ui/action",
            _actionApi.get_new_instance ());

         _state.core->register_interface (
            "dmz/ui/inputDialog",
            _inputDialogApi.get_new_instance ());
      }

      _allowMultipleStr =
         V8StringPersist::New (v8::String::NewSymbol ("allowMultiple"));
      _allowedAreasStr = V8StringPersist::New (v8::String::NewSymbol ("allowedAreas"));
      _areaStr = V8StringPersist::New (v8::String::NewSymbol ("area"));
      _captionStr = V8StringPersist::New (v8::String::NewSymbol ("caption"));
      _defaultButtonStr =
         V8StringPersist::New (v8::String::NewSymbol ("defaultButton"));
      _detailedTextStr = V8StringPersist::New (v8::String::NewSymbol ("detailedText"));
      _dirStr = V8StringPersist::New (v8::String::NewSymbol ("dir"));
      _featuresStr = V8StringPersist::New (v8::String::NewSymbol ("features"));
      _filterStr = V8StringPersist::New (v8::String::NewSymbol ("filter"));
      _floatingStr = V8StringPersist::New (v8::String::NewSymbol ("floating"));
      _infoTextStr = V8StringPersist::New (v8::String::NewSymbol ("informativeText"));
      _optionsStr = V8StringPersist::New (v8::String::NewSymbol ("options"));
      _standardButtonsStr =
         V8StringPersist::New (v8::String::NewSymbol ("standardButtons"));
      _statusTipStr = V8StringPersist::New (v8::String::NewSymbol ("statusTip"));
      _textStr = V8StringPersist::New (v8::String::NewSymbol ("text"));
      _toolTipStr = V8StringPersist::New (v8::String::NewSymbol ("toolTip"));
      _typeStr = V8StringPersist::New (v8::String::NewSymbol ("type"));
      _visibleStr = V8StringPersist::New (v8::String::NewSymbol ("visible"));
      _currentStr = V8StringPersist::New (v8::String::NewSymbol ("current"));
      _decimalStr = V8StringPersist::New (v8::String::NewSymbol ("decimal"));
      _itemsStr = V8StringPersist::New (v8::String::NewSymbol ("items"));
      _labelStr = V8StringPersist::New (v8::String::NewSymbol ("label"));
      _maxStr = V8StringPersist::New (v8::String::NewSymbol ("max"));
      _minStr = V8StringPersist::New (v8::String::NewSymbol ("min"));
      _modeStr = V8StringPersist::New (v8::String::NewSymbol ("mode"));
      _stepStr = V8StringPersist::New (v8::String::NewSymbol ("step"));
      _titleStr = V8StringPersist::New (v8::String::NewSymbol ("title"));
      _valueStr = V8StringPersist::New (v8::String::NewSymbol ("value"));
      _shortcutStr = V8StringPersist::New (v8::String::NewSymbol ("shortcut"));
      _iconStr = V8StringPersist::New (v8::String::NewSymbol ("icon"));
      _editableStr = V8StringPersist::New (v8::String::NewSymbol ("editable"));

      // create v8 wrapper for the main window and set delete object to false -ss
      if (_state.mainWindowModule) {

         QMainWindow *mainWindow = _state.mainWindowModule->get_qt_main_window ();
         if (mainWindow) {

            V8Value value = create_v8_qwidget (mainWindow);

            V8QtObject *vobj = _to_v8_qt_object (value);
            if (vobj) { vobj->set_delete_object (False); }
            else { _log.error << "No V8QtObject found for QMainWindow!" << endl; }
         }
      }
   }
   else if (State == JsExtV8::Init) {

      if (!_mainWindowState.isEmpty () && _state.mainWindowModule) {

         QMainWindow *mainWindow = _state.mainWindowModule->get_qt_main_window ();
         if (mainWindow) { mainWindow->restoreState (_mainWindowState); }
      }
   }
   else if (State == JsExtV8::Stop) {

   }
   else if (State == JsExtV8::Shutdown) {

      foreach (QWidget *dialog, _dialogList) { dialog->close (); }
      _dialogList.clear ();

      if (_state.mainWindowModule) {

         _mainWindowState.clear ();

         QMainWindow *mainWindow = _state.mainWindowModule->get_qt_main_window ();
         if (mainWindow) {

            _mainWindowState = mainWindow->saveState ();

            if (_centralWidget) {

               mainWindow->setCentralWidget (0);
               _centralWidget->setParent (0);
            }
         }

         foreach (String dock, _dockList) {

            _state.mainWindowModule->remove_dock_widget (dock);
         }
      }

      _dockList.clear ();

      if (_objectMap.count ()) {

         QMapIterator<QObject *, V8QtObject *> it (_objectMap);
         while (it.hasNext ()) {

            it.next ();

            V8QtObject *obj = it.value ();
            delete obj; obj = 0;
         }

         _objectMap.clear ();
      }

      while (!_valueDeleteList.isEmpty ()) {

         V8ValueRef *value = _valueDeleteList.takeFirst ();
         delete value; value = 0;
      }

      _objectCtor.Dispose (); _objectCtor.Clear ();
      _widgetCtor.Dispose (); _widgetCtor.Clear ();
      _frameCtor.Dispose (); _frameCtor.Clear ();
      _groupBoxCtor.Dispose (); _groupBoxCtor.Clear ();
      _buttonCtor.Dispose (); _buttonCtor.Clear ();
      _listWidgetItemCtor.Dispose (); _listWidgetItemCtor.Clear ();
      _listWidgetCtor.Dispose (); _listWidgetCtor.Clear ();
      _spinBoxCtor.Dispose (); _spinBoxCtor.Clear ();
      _comboBoxCtor.Dispose (); _comboBoxCtor.Clear ();
      _sliderCtor.Dispose (); _sliderCtor.Clear ();
      _dialCtor.Dispose (); _dialCtor.Clear ();
      _lineEditCtor.Dispose (); _lineEditCtor.Clear ();
      _textEditCtor.Dispose (); _textEditCtor.Clear ();
      _labelCtor.Dispose (); _labelCtor.Clear ();
      _progressBarCtor.Dispose (); _progressBarCtor.Clear ();
      _dialogCtor.Dispose (); _dialogCtor.Clear ();
      _lcdNumberCtor.Dispose (); _lcdNumberCtor.Clear ();
      _stackedCtor.Dispose (); _stackedCtor.Clear ();
      _tabCtor.Dispose (); _tabCtor.Clear ();
      _hBoxLayoutCtor.Dispose (); _hBoxLayoutCtor.Clear ();
      _vBoxLayoutCtor.Dispose (); _vBoxLayoutCtor.Clear ();
      _boxLayoutCtor.Dispose (); _boxLayoutCtor.Clear ();
      _formLayoutCtor.Dispose (); _formLayoutCtor.Clear ();
      _gridLayoutCtor.Dispose (); _gridLayoutCtor.Clear ();
      _layoutCtor.Dispose (); _layoutCtor.Clear ();
      _mainWindowCtor.Dispose (); _mainWindowCtor.Clear ();
      _dockWidgetCtor.Dispose (); _dockWidgetCtor.Clear ();
      _dteCtor.Dispose (); _dteCtor.Clear ();
      _actionCtor.Dispose (); _actionCtor.Clear ();
      _treeWidgetCtor.Dispose (); _treeWidgetCtor.Clear ();
      _treeWidgetItemCtor.Dispose (); _treeWidgetItemCtor.Clear ();
      _tableWidgetCtor.Dispose (); _tableWidgetCtor.Clear ();
      _tableWidgetItemCtor.Dispose (); _tableWidgetItemCtor.Clear ();
      _tableSelectionCtor.Dispose (); _tableSelectionCtor.Clear ();
      _toolboxWidgetCtor.Dispose (); _toolboxWidgetCtor.Clear ();
      _messageboxCtor.Dispose (); _messageboxCtor.Clear ();
      _inputDialogCtor.Dispose (); _inputDialogCtor.Dispose ();

      _allowMultipleStr.Dispose (); _allowMultipleStr.Clear ();
      _allowedAreasStr.Dispose (); _allowedAreasStr.Clear ();
      _areaStr.Dispose (); _areaStr.Clear ();
      _captionStr.Dispose (); _captionStr.Clear ();
      _defaultButtonStr.Dispose (); _defaultButtonStr.Clear ();
      _detailedTextStr.Dispose (); _detailedTextStr.Clear ();
      _dirStr.Dispose (); _dirStr.Clear ();
      _filterStr.Dispose (); _filterStr.Clear ();
      _floatingStr.Dispose (); _floatingStr.Clear ();
      _infoTextStr.Dispose (); _infoTextStr.Clear ();
      _optionsStr.Dispose (); _optionsStr.Clear ();
      _standardButtonsStr.Dispose (); _standardButtonsStr.Clear ();
      _statusTipStr.Dispose (); _statusTipStr.Clear ();
      _textStr.Dispose (); _textStr.Clear ();
      _toolTipStr.Dispose (); _toolTipStr.Clear ();
      _typeStr.Dispose (); _typeStr.Clear ();
      _visibleStr.Dispose (); _visibleStr.Clear ();
      _currentStr.Dispose (); _currentStr.Clear ();
      _decimalStr.Dispose (); _decimalStr.Clear ();
      _itemsStr.Dispose (); _itemsStr.Clear ();
      _labelStr.Dispose (); _labelStr.Clear ();
      _maxStr.Dispose (); _maxStr.Clear ();
      _minStr.Dispose (); _minStr.Clear ();
      _modeStr.Dispose (); _modeStr.Clear ();
      _stepStr.Dispose (); _stepStr.Clear ();
      _titleStr.Dispose (); _titleStr.Clear ();
      _valueStr.Dispose (); _valueStr.Clear ();
      _shortcutStr.Dispose (); _shortcutStr.Clear ();
      _iconStr.Dispose (); _iconStr.Clear ();
      _editableStr.Dispose (); _editableStr.Clear ();

      _self.Dispose (); _self.Clear ();

      _qtApi.clear ();
      _uiLoaderApi.clear ();
      _frameApi.clear ();
      _mainWindowApi.clear ();
      _dockWidgetApi.clear ();
      _messageBoxApi.clear ();
      _layoutApi.clear ();
      _fileDialogApi.clear ();
      _groupBoxApi.clear ();
      _actionApi.clear ();
      _inputDialogApi.clear ();
      _state.context.Clear ();

      _obsTable.empty ();
   }
}


void
dmz::JsModuleUiV8QtBasic::release_js_instance_v8 (
      const Handle InstanceHandle,
      const String &InstanceName,
      v8::Handle<v8::Object> &instance) {

   ObsStruct *os = _obsTable.remove (InstanceHandle);
   if (os) {

      foreach (V8QtObject *obj, os->list) {

         obj->release_callback (InstanceHandle);
      }

      delete os; os = 0;
   }
}


bool
dmz::JsModuleUiV8QtBasic::eventFilter (QObject *watched, QEvent *event) {

   bool result (false);

   if (event->type () == QEvent::ShortcutOverride) {

      QKeyEvent* ke = static_cast<QKeyEvent*>(event);

      // ignore undo/redo events
      if (ke == QKeySequence::Undo || ke == QKeySequence::Redo) { result = true; }
   }

   // standard event processing
   if (!result) { result = QObject::eventFilter (watched, event); }

   return result;
}


void
dmz::JsModuleUiV8QtBasic::v8_qt_widget_destroyed (V8QtWidget *widget) {

   if (widget) {

      // remove object from all observer lists
      HashTableHandleIterator it;
      ObsStruct *os = _obsTable.get_first (it);
      while (os) {

         os->list.removeAll (widget);
         os = _obsTable.get_next (it);
      }

      // remove from dialog list
      _dialogList.removeAll (widget->get_qwidget ());

      // remove from object map
      _objectMap.remove (widget->get_qobject ());
   }
}


dmz::V8QtWidget *
dmz::JsModuleUiV8QtBasic::_to_v8_qt_widget (V8Value value) {

   V8QtWidget *result (0);
   V8QtObject *object = _to_v8_qt_object (value);
   if (object) { result = qobject_cast<V8QtWidget *>(object); }
   return result;
}


dmz::V8QtObject *
dmz::JsModuleUiV8QtBasic::_to_v8_qt_object (V8Value value) {

   v8::HandleScope scope;
   V8QtObject *result (0);

   V8Object obj = v8_to_object (value);
   if (!obj.IsEmpty ()) {

      if (_objectTemp->HasInstance (obj)) {

         result = (V8QtObject *)v8::External::Unwrap (obj->GetInternalField (0));
      }
   }

   return result;
}


// JsModuleUiV8QtBasic Interface

QVariant
dmz::JsModuleUiV8QtBasic::_qvariant_wrap_v8 (V8Value value) {

   v8::HandleScope scope;
   QVariant result;

   V8ValueRef *ref = new V8ValueRef (value, _valueDeleteList);
   V8Variant variant (ref);

   scope.Close (ref->value);
   ref->unref (); ref = 0;

   result.setValue (variant);
   return result;
}


dmz::V8Value
dmz::JsModuleUiV8QtBasic::_qvariant_unwrap_v8 (const QVariant &Value) {

   v8::HandleScope scope;
   V8Value result = v8::Undefined ();

   V8Variant variant = Value.value<V8Variant>();
   if (variant.valueRef) { result = variant.valueRef->value; }

   return scope.Close (result);
}


dmz::String
dmz::JsModuleUiV8QtBasic::_find_ui_file (const String &Name) {

   String result;

   if (!find_file (_searchPaths, Name, result)) {

      find_file (_searchPaths, Name + ".ui", result);
   }

   return result;
}


void
dmz::JsModuleUiV8QtBasic::_init (Config &local) {

   setObjectName (get_plugin_name ().get_buffer ());

   _searchPaths = config_to_path_string_container (local);

   _mainWindowModuleName = config_to_string (
      "main-window-module.name",
      local,
      "dmzQtModuleMainWindowBasic");

   v8::HandleScope scope;

   _self = V8ValuePersist::New (v8::External::Wrap (this));

   // Qt API
   // enum Qt::AlignmentFlag
   _qtApi.add_constant ("AlignLeft", (UInt32)Qt::AlignLeft);
   _qtApi.add_constant ("AlignRight", (UInt32)Qt::AlignRight);
   _qtApi.add_constant ("AlignHCenter", (UInt32)Qt::AlignHCenter);
   _qtApi.add_constant ("AlignJustify", (UInt32)Qt::AlignJustify);
   _qtApi.add_constant ("AlignTop", (UInt32)Qt::AlignTop);
   _qtApi.add_constant ("AlignBottom", (UInt32)Qt::AlignBottom);
   _qtApi.add_constant ("AlignVCenter", (UInt32)Qt::AlignVCenter);
   _qtApi.add_constant ("AlignCenter", (UInt32)Qt::AlignCenter);

   // enum Qt::CaseSensitivity
   _qtApi.add_constant ("CaseInsensitive", (UInt32)Qt::CaseInsensitive);
   _qtApi.add_constant ("CaseSensitive", (UInt32)Qt::CaseSensitive);

   // enum Qt::CheckState
   _qtApi.add_constant ("Unchecked", (UInt32)Qt::Unchecked);
   _qtApi.add_constant ("PartiallyChecked", (UInt32)Qt::PartiallyChecked);
   _qtApi.add_constant ("Checked", (UInt32)Qt::Checked);

   // enum Qt::Corner
   _qtApi.add_constant ("TopLeftCorner", (UInt32)Qt::TopLeftCorner);
   _qtApi.add_constant ("TopRightCorner", (UInt32)Qt::TopRightCorner);
   _qtApi.add_constant ("BottomLeftCorner", (UInt32)Qt::BottomLeftCorner);
   _qtApi.add_constant ("BottomRightCorner", (UInt32)Qt::BottomRightCorner);

   // enum Qt::DockWidgetArea
   _qtApi.add_constant ("LeftDockWidgetArea", (UInt32)Qt::LeftDockWidgetArea);
   _qtApi.add_constant ("RightDockWidgetArea", (UInt32)Qt::RightDockWidgetArea);
   _qtApi.add_constant ("TopDockWidgetArea", (UInt32)Qt::TopDockWidgetArea);
   _qtApi.add_constant ("LeftDockWidgetArea", (UInt32)Qt::LeftDockWidgetArea);
   _qtApi.add_constant ("BottomDockWidgetArea", (UInt32)Qt::BottomDockWidgetArea);
   _qtApi.add_constant ("AllDockWidgetAreas", (UInt32)Qt::AllDockWidgetAreas);
   _qtApi.add_constant ("NoDockWidgetArea", (UInt32)Qt::NoDockWidgetArea);

   // Qt's predefined QColor objects:
   _qtApi.add_constant ("white", (UInt32)Qt::white);
   _qtApi.add_constant ("black", (UInt32)Qt::black);
   _qtApi.add_constant ("red", (UInt32)Qt::red);
   _qtApi.add_constant ("darkRed", (UInt32)Qt::darkRed);
   _qtApi.add_constant ("green", (UInt32)Qt::green);
   _qtApi.add_constant ("darkGreen", (UInt32)Qt::darkGreen);
   _qtApi.add_constant ("blue", (UInt32)Qt::blue);
   _qtApi.add_constant ("darkBlue", (UInt32)Qt::darkBlue);
   _qtApi.add_constant ("cyan", (UInt32)Qt::cyan);
   _qtApi.add_constant ("darkCyan", (UInt32)Qt::darkCyan);
   _qtApi.add_constant ("magenta", (UInt32)Qt::magenta);
   _qtApi.add_constant ("darkMagenta", (UInt32)Qt::darkMagenta);
   _qtApi.add_constant ("yellow", (UInt32)Qt::yellow);
   _qtApi.add_constant ("darkYellow", (UInt32)Qt::darkYellow);
   _qtApi.add_constant ("gray", (UInt32)Qt::gray);
   _qtApi.add_constant ("darkGray", (UInt32)Qt::darkGray);
   _qtApi.add_constant ("lightGray", (UInt32)Qt::lightGray);
   _qtApi.add_constant ("transparent", (UInt32)Qt::transparent);

   // enum Qt::Orientation
   _qtApi.add_constant ("Horizontal", (UInt32)Qt::Horizontal);
   _qtApi.add_constant ("Vertical", (UInt32)Qt::Vertical);

   // enum Qt::SortOrder
   _qtApi.add_constant ("AscendingOrder", (UInt32)Qt::AscendingOrder);
   _qtApi.add_constant ("DescendingOrder", (UInt32)Qt::DescendingOrder);

   // enum Qt::ToolBarArea
   _qtApi.add_constant ("LeftToolBarArea", (UInt32)Qt::LeftToolBarArea);
   _qtApi.add_constant ("RightToolBarArea", (UInt32)Qt::RightToolBarArea);
   _qtApi.add_constant ("TopToolBarArea", (UInt32)Qt::TopToolBarArea);
   _qtApi.add_constant ("BottomToolBarArea", (UInt32)Qt::BottomToolBarArea);
   _qtApi.add_constant ("AllToolBarAreas", (UInt32)Qt::AllToolBarAreas);
   _qtApi.add_constant ("NoToolBarArea", (UInt32)Qt::NoToolBarArea);

   // enum Qt::MatchFlag
   _qtApi.add_constant ("MatchExactly", (UInt32)Qt::MatchExactly);
   _qtApi.add_constant ("MatchFixedString", (UInt32)Qt::MatchFixedString);
   _qtApi.add_constant ("MatchContains", (UInt32)Qt::MatchContains);
   _qtApi.add_constant ("MatchStartsWith", (UInt32)Qt::MatchStartsWith);
   _qtApi.add_constant ("MatchEndsWith", (UInt32)Qt::MatchEndsWith);
   _qtApi.add_constant ("MatchCaseSensitive", (UInt32)Qt::MatchCaseSensitive);
   _qtApi.add_constant ("MatchRegExp", (UInt32)Qt::MatchRegExp);
   _qtApi.add_constant ("MatchWildcard", (UInt32)Qt::MatchWildcard);
   _qtApi.add_constant ("MatchWrap", (UInt32)Qt::MatchWrap);
   _qtApi.add_constant ("MatchRecursive", (UInt32)Qt::MatchRecursive);

   // enum QLineEdit::EchoMode
   _qtApi.add_constant ("Normal", (UInt32)QLineEdit::Normal);
   _qtApi.add_constant ("Password", (UInt32)QLineEdit::Password);

   // UiLoader API
   _uiLoaderApi.add_function ("load", _uiloader_load, _self);

   _init_object ();
   _init_widget ();
   _init_group_box ();
   _init_frame ();
   _init_button ();
   _init_tree_widget_item ();
   _init_tree_widget ();
   _init_list_widget_item ();
   _init_list_widget ();
   _init_table_widget_item ();
   _init_table_widget ();
   _init_table_selection ();
   _init_toolbox ();
   _init_spinbox ();
   _init_combobox ();
   _init_slider ();
   _init_lineEdit ();
   _init_textEdit ();
   _init_dial ();
   _init_label ();
   _init_progressBar ();
   _init_dialog ();
   _init_message_box ();
   _init_lcdNumber ();
   _init_stacked_widget ();
   _init_tab_widget ();
   _init_layout ();
   _init_box_layout ();
   _init_hbox_layout ();
   _init_vbox_layout ();
   _init_grid_layout ();
   _init_form_layout ();
   _init_file_dialog ();
   _init_main_window ();
   _init_dock_widget ();
   _init_dt ();
   _init_action ();
   _init_input_dialog ();
}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzJsModuleUiV8QtBasic (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::JsModuleUiV8QtBasic (Info, local);
}

};
