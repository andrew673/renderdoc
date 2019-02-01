/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2017-2019 Baldur Karlsson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#include "SettingsDialog.h"
#include <QKeyEvent>
#include <QTextEdit>
#include <QToolButton>
#include "Code/Interface/QRDInterface.h"
#include "Code/QRDUtils.h"
#include "Styles/StyleData.h"
#include "Widgets/OrderedListEditor.h"
#include "CaptureDialog.h"
#include "ui_SettingsDialog.h"

SettingsDialog::SettingsDialog(ICaptureContext &ctx, QWidget *parent)
    : QDialog(parent), ui(new Ui::SettingsDialog), m_Ctx(ctx)
{
  ui->setupUi(this);

  QString styleChooseTooltip = ui->UIStyle->toolTip();

  for(int i = 0; i < StyleData::numAvailable; i++)
    styleChooseTooltip += lit("<br>- ") + StyleData::availStyles[i].styleDescription;

  ui->UIStyle->setToolTip(styleChooseTooltip);
  ui->UIStyle_label->setToolTip(styleChooseTooltip);

  for(int i = 0; i < StyleData::numAvailable; i++)
    ui->UIStyle->addItem(StyleData::availStyles[i].styleName);

  setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

  ui->tabWidget->tabBar()->setVisible(false);

  for(int i = 0; i < ui->tabWidget->count(); i++)
    ui->pages->addItem(ui->tabWidget->tabText(i));

  m_Init = true;

  for(int i = 0; i < (int)TimeUnit::Count; i++)
  {
    ui->EventBrowser_TimeUnit->addItem(UnitSuffix((TimeUnit)i));
  }

  ui->pages->clearSelection();
  ui->pages->setItemSelected(ui->pages->item(0), true);
  ui->tabWidget->setCurrentIndex(0);

  ui->pages->setMinimumWidth(ui->pages->sizeHintForColumn(0));
  ui->pages->adjustSize();

  for(int i = 0; i < StyleData::numAvailable; i++)
  {
    if(StyleData::availStyles[i].styleID == m_Ctx.Config().UIStyle)
    {
      ui->UIStyle->setCurrentIndex(i);
      break;
    }
  }

  ui->saveDirectory->setText(m_Ctx.Config().DefaultCaptureSaveDirectory);
  ui->tempDirectory->setText(m_Ctx.Config().TemporaryCaptureDirectory);

  ui->shaderTools->setColumnCount(2);
  ui->shaderTools->setHorizontalHeaderLabels(QStringList() << tr("Tool") << tr("Process"));

  ui->shaderTools->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
  ui->shaderTools->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

  for(const ShaderProcessingTool &tool : m_Ctx.Config().ShaderProcessors)
    addProcessor(tool);

  ui->shaderTools->horizontalHeader()->resizeSection(0, 100);

  ui->shaderTools->verticalHeader()->setSectionsMovable(true);
  ui->shaderTools->verticalHeader()->setMinimumWidth(20);

  ui->deleteShaderTool->setEnabled(false);
  ui->editShaderTool->setEnabled(false);

  ui->ExternalTool_RGPIntegration->setChecked(m_Ctx.Config().ExternalTool_RGPIntegration);
  ui->ExternalTool_RadeonGPUProfiler->setText(m_Ctx.Config().ExternalTool_RadeonGPUProfiler);

  ui->Android_SDKPath->setText(m_Ctx.Config().Android_SDKPath);
  ui->Android_JDKPath->setText(m_Ctx.Config().Android_JDKPath);
  ui->Android_MaxConnectTimeout->setValue(m_Ctx.Config().Android_MaxConnectTimeout);

  ui->TextureViewer_ResetRange->setChecked(m_Ctx.Config().TextureViewer_ResetRange);
  ui->TextureViewer_PerTexSettings->setChecked(m_Ctx.Config().TextureViewer_PerTexSettings);
  ui->ShaderViewer_FriendlyNaming->setChecked(m_Ctx.Config().ShaderViewer_FriendlyNaming);
  ui->CheckUpdate_AllowChecks->setChecked(m_Ctx.Config().CheckUpdate_AllowChecks);
  ui->Font_PreferMonospaced->setChecked(m_Ctx.Config().Font_PreferMonospaced);

  ui->AlwaysReplayLocally->setChecked(m_Ctx.Config().AlwaysReplayLocally);

#if RENDERDOC_ANALYTICS_ENABLE
  if(m_Ctx.Config().Analytics_TotalOptOut)
  {
    ui->analyticsAutoSubmit->setChecked(false);
    ui->analyticsManualCheck->setChecked(false);
    ui->analyticsOptOut->setChecked(true);

    // once we've started with analytics disabled, only a restart can re-enable them.
    ui->analyticsAutoSubmit->setText(ui->analyticsAutoSubmit->text() + tr(" (Requires Restart)"));
    ui->analyticsManualCheck->setText(ui->analyticsManualCheck->text() + tr(" (Requires Restart)"));
  }
  else if(m_Ctx.Config().Analytics_ManualCheck)
  {
    ui->analyticsAutoSubmit->setChecked(false);
    ui->analyticsManualCheck->setChecked(true);
    ui->analyticsOptOut->setChecked(false);
  }
  else
  {
    ui->analyticsAutoSubmit->setChecked(true);
    ui->analyticsManualCheck->setChecked(false);
    ui->analyticsOptOut->setChecked(false);
  }
#else
  ui->analyticsDescribeLabel->setText(tr("Analytics was disabled at compile time."));

  ui->analyticsAutoSubmit->setEnabled(false);
  ui->analyticsManualCheck->setEnabled(false);
  ui->analyticsOptOut->setEnabled(false);
#endif

  ui->AllowGlobalHook->setChecked(m_Ctx.Config().AllowGlobalHook);

  ui->EventBrowser_TimeUnit->setCurrentIndex((int)m_Ctx.Config().EventBrowser_TimeUnit);
  ui->EventBrowser_AddFake->setChecked(m_Ctx.Config().EventBrowser_AddFake);
  ui->EventBrowser_HideEmpty->setChecked(m_Ctx.Config().EventBrowser_HideEmpty);
  ui->EventBrowser_HideAPICalls->setChecked(m_Ctx.Config().EventBrowser_HideAPICalls);
  ui->EventBrowser_ApplyColors->setChecked(m_Ctx.Config().EventBrowser_ApplyColors);
  ui->EventBrowser_ColorEventRow->setChecked(m_Ctx.Config().EventBrowser_ColorEventRow);

  // disable sub-checkbox
  ui->EventBrowser_ColorEventRow->setEnabled(ui->EventBrowser_ApplyColors->isChecked());

  ui->Comments_ShowOnLoad->setChecked(m_Ctx.Config().Comments_ShowOnLoad);

  ui->Formatter_MinFigures->setValue(m_Ctx.Config().Formatter_MinFigures);
  ui->Formatter_MaxFigures->setValue(m_Ctx.Config().Formatter_MaxFigures);
  ui->Formatter_NegExp->setValue(m_Ctx.Config().Formatter_NegExp);
  ui->Formatter_PosExp->setValue(m_Ctx.Config().Formatter_PosExp);

  if(!RENDERDOC_CanGlobalHook())
  {
    ui->AllowGlobalHook->setEnabled(false);

    QString disabledTooltip = tr("Global hooking is not supported on this platform");
    ui->AllowGlobalHook->setToolTip(disabledTooltip);
    ui->globalHookLabel->setToolTip(disabledTooltip);
  }

  m_Init = false;

  QObject::connect(ui->shaderTools->verticalHeader(), &QHeaderView::sectionMoved, this,
                   &SettingsDialog::shaderTools_rowMoved);
  QObject::connect(ui->Formatter_MinFigures, OverloadedSlot<int>::of(&QSpinBox::valueChanged), this,
                   &SettingsDialog::formatter_valueChanged);
  QObject::connect(ui->Formatter_MaxFigures, OverloadedSlot<int>::of(&QSpinBox::valueChanged), this,
                   &SettingsDialog::formatter_valueChanged);
  QObject::connect(ui->Formatter_NegExp, OverloadedSlot<int>::of(&QSpinBox::valueChanged), this,
                   &SettingsDialog::formatter_valueChanged);
  QObject::connect(ui->Formatter_PosExp, OverloadedSlot<int>::of(&QSpinBox::valueChanged), this,
                   &SettingsDialog::formatter_valueChanged);
}

SettingsDialog::~SettingsDialog()
{
  delete ui;
}

void SettingsDialog::focusItem(QString item)
{
  for(int i = 0; i < ui->tabWidget->count(); i++)
  {
    QWidget *w = ui->tabWidget->widget(i)->findChild<QWidget *>(item);

    if(w)
    {
      ui->tabWidget->setCurrentIndex(i);
      w->setFocus(Qt::MouseFocusReason);
      return;
    }
  }

  qCritical() << "Couldn't find" << item << "to focus on settings dialog";
}

void SettingsDialog::on_pages_itemSelectionChanged()
{
  QList<QListWidgetItem *> sel = ui->pages->selectedItems();

  if(sel.empty())
  {
    ui->pages->setItemSelected(ui->pages->item(ui->tabWidget->currentIndex()), true);
  }
  else
  {
    ui->tabWidget->setCurrentIndex(ui->pages->row(sel[0]));
  }
}

void SettingsDialog::on_okButton_accepted()
{
  setResult(1);
  accept();
}

// general
void SettingsDialog::formatter_valueChanged(int val)
{
  m_Ctx.Config().Formatter_MinFigures = ui->Formatter_MinFigures->value();
  m_Ctx.Config().Formatter_MaxFigures = ui->Formatter_MaxFigures->value();
  m_Ctx.Config().Formatter_NegExp = ui->Formatter_NegExp->value();
  m_Ctx.Config().Formatter_PosExp = ui->Formatter_PosExp->value();

  m_Ctx.Config().SetupFormatting();

  m_Ctx.Config().Save();
}

void SettingsDialog::on_tempDirectory_textEdited(const QString &dir)
{
  if(QDir(dir).exists())
    m_Ctx.Config().TemporaryCaptureDirectory = dir;
  else
    m_Ctx.Config().TemporaryCaptureDirectory = QString();

  m_Ctx.Config().Save();
}

void SettingsDialog::on_saveDirectory_textEdited(const QString &dir)
{
  if(QDir(dir).exists() || dir.isEmpty())
    m_Ctx.Config().DefaultCaptureSaveDirectory = dir;

  m_Ctx.Config().Save();
}

void SettingsDialog::on_browseSaveCaptureDirectory_clicked()
{
  QString dir =
      RDDialog::getExistingDirectory(this, tr("Choose default directory for saving captures"),
                                     m_Ctx.Config().DefaultCaptureSaveDirectory);

  if(!dir.isEmpty())
  {
    m_Ctx.Config().DefaultCaptureSaveDirectory = dir;
    ui->saveDirectory->setText(dir);
  }

  m_Ctx.Config().Save();
}

void SettingsDialog::on_AllowGlobalHook_toggled(bool checked)
{
  m_Ctx.Config().AllowGlobalHook = ui->AllowGlobalHook->isChecked();

  m_Ctx.Config().Save();

  if(m_Ctx.HasCaptureDialog())
    m_Ctx.GetCaptureDialog()->UpdateGlobalHook();
}

void SettingsDialog::on_CheckUpdate_AllowChecks_toggled(bool checked)
{
  m_Ctx.Config().CheckUpdate_AllowChecks = ui->CheckUpdate_AllowChecks->isChecked();

  if(!m_Ctx.Config().CheckUpdate_AllowChecks)
  {
    m_Ctx.Config().CheckUpdate_UpdateAvailable = false;
    m_Ctx.Config().CheckUpdate_UpdateResponse = "";
  }

  m_Ctx.Config().Save();
}

void SettingsDialog::on_Font_PreferMonospaced_toggled(bool checked)
{
  m_Ctx.Config().Font_PreferMonospaced = ui->Font_PreferMonospaced->isChecked();

  m_Ctx.Config().SetupFormatting();

  m_Ctx.Config().Save();
}

void SettingsDialog::on_AlwaysReplayLocally_toggled(bool checked)
{
  m_Ctx.Config().AlwaysReplayLocally = ui->AlwaysReplayLocally->isChecked();

  m_Ctx.Config().Save();
}

void SettingsDialog::on_analyticsAutoSubmit_toggled(bool checked)
{
  if(checked)
  {
    m_Ctx.Config().Analytics_ManualCheck = false;
    m_Ctx.Config().Analytics_TotalOptOut = false;

    m_Ctx.Config().Save();
  }
}

void SettingsDialog::on_analyticsManualCheck_toggled(bool checked)
{
  if(checked)
  {
    m_Ctx.Config().Analytics_ManualCheck = true;
    m_Ctx.Config().Analytics_TotalOptOut = false;

    m_Ctx.Config().Save();
  }
}

void SettingsDialog::on_analyticsOptOut_toggled(bool checked)
{
  if(checked)
  {
    m_Ctx.Config().Analytics_ManualCheck = false;
    m_Ctx.Config().Analytics_TotalOptOut = true;

    // immediately disable the analytics collection and ensure it can't send any reports.
    Analytics::Disable();

    m_Ctx.Config().Save();
  }
}

void SettingsDialog::on_analyticsDescribeLabel_linkActivated(const QString &link)
{
  Analytics::DocumentReport();
}

// core
void SettingsDialog::on_chooseSearchPaths_clicked()
{
  QDialog listEditor;

  listEditor.setWindowTitle(tr("Shader debug info search paths"));
  listEditor.setWindowFlags(listEditor.windowFlags() & ~Qt::WindowContextHelpButtonHint);

  OrderedListEditor list(tr("Search Path"), BrowseMode::Folder);

  QVBoxLayout layout;
  QDialogButtonBox okCancel;
  okCancel.setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
  layout.addWidget(&list);
  layout.addWidget(&okCancel);

  QObject::connect(&okCancel, &QDialogButtonBox::accepted, &listEditor, &QDialog::accept);
  QObject::connect(&okCancel, &QDialogButtonBox::rejected, &listEditor, &QDialog::reject);

  listEditor.setLayout(&layout);

  QString setting = m_Ctx.Config().GetConfigSetting("shader.debug.searchPaths");

  list.setItems(setting.split(QLatin1Char(';'), QString::SkipEmptyParts));

  int res = RDDialog::show(&listEditor);

  if(res)
    m_Ctx.Config().SetConfigSetting(lit("shader.debug.searchPaths"),
                                    list.getItems().join(QLatin1Char(';')));
}

void SettingsDialog::on_ExternalTool_RGPIntegration_toggled(bool checked)
{
  m_Ctx.Config().ExternalTool_RGPIntegration = checked;

  m_Ctx.Config().Save();
}

void SettingsDialog::on_ExternalTool_RadeonGPUProfiler_textEdited(const QString &rgp)
{
  if(QFileInfo::exists(rgp) || rgp.isEmpty())
    m_Ctx.Config().ExternalTool_RadeonGPUProfiler = rgp;

  m_Ctx.Config().Save();
}

void SettingsDialog::on_browseRGPPath_clicked()
{
  QString rgp = RDDialog::getExecutableFileName(
      this, tr("Locate RGP executable"),
      QFileInfo(m_Ctx.Config().ExternalTool_RadeonGPUProfiler).absoluteDir().path());

  if(!rgp.isEmpty())
  {
    ui->ExternalTool_RadeonGPUProfiler->setText(rgp);
    m_Ctx.Config().ExternalTool_RadeonGPUProfiler = rgp;
  }

  m_Ctx.Config().Save();
}

// texture viewer
void SettingsDialog::on_TextureViewer_PerTexSettings_toggled(bool checked)
{
  m_Ctx.Config().TextureViewer_PerTexSettings = ui->TextureViewer_PerTexSettings->isChecked();

  m_Ctx.Config().Save();
}

void SettingsDialog::on_TextureViewer_ResetRange_toggled(bool checked)
{
  m_Ctx.Config().TextureViewer_ResetRange = ui->TextureViewer_ResetRange->isChecked();

  m_Ctx.Config().Save();
}

// shader viewer
void SettingsDialog::on_ShaderViewer_FriendlyNaming_toggled(bool checked)
{
  m_Ctx.Config().ShaderViewer_FriendlyNaming = ui->ShaderViewer_FriendlyNaming->isChecked();

  m_Ctx.Config().Save();
}

void SettingsDialog::addProcessor(const ShaderProcessingTool &tool)
{
  int row = ui->shaderTools->rowCount();
  ui->shaderTools->insertRow(row);

  ui->shaderTools->setVerticalHeaderItem(row, new QTableWidgetItem(QString()));

  ui->shaderTools->setItem(row, 0, new QTableWidgetItem(tool.name));
  ui->shaderTools->setItem(
      row, 1,
      new QTableWidgetItem(QFormatStr("%1 -> %2").arg(ToQStr(tool.input)).arg(ToQStr(tool.output))));
}

bool SettingsDialog::editTool(int existing, ShaderProcessingTool &tool)
{
  QDialog dialog;
  dialog.setWindowFlags(dialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
  dialog.setWindowTitle(tr("Configure Shader Processing Tool"));

  dialog.resize(400, 0);

  QGridLayout grid(&dialog);

  QLabel *lab;

  lab = new QLabel(tr("Name:"), &dialog);
  lab->setAlignment(Qt::AlignRight | Qt::AlignTop);
  grid.addWidget(lab, 0, 0, 1, 1);

  lab = new QLabel(tr("Tool Type:"), &dialog);
  lab->setAlignment(Qt::AlignRight | Qt::AlignTop);
  grid.addWidget(lab, 1, 0, 1, 1);

  lab = new QLabel(tr("Executable:"), &dialog);
  lab->setAlignment(Qt::AlignRight | Qt::AlignTop);
  grid.addWidget(lab, 2, 0, 1, 1);

  lab = new QLabel(tr("Command Line:"), &dialog);
  lab->setAlignment(Qt::AlignRight | Qt::AlignTop);
  grid.addWidget(lab, 3, 0, 1, 1);

  lab = new QLabel(tr("Input/Output:"), &dialog);
  lab->setAlignment(Qt::AlignRight | Qt::AlignTop);
  grid.addWidget(lab, 4, 0, 1, 1);

  QLineEdit nameEdit;
  nameEdit.setPlaceholderText(lit("Tool Name"));
  nameEdit.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  nameEdit.setMinimumHeight(20);

  QStringList strs;

  for(KnownShaderTool t : values<KnownShaderTool>())
  {
    if(t == KnownShaderTool::Unknown)
      strs << tr("Custom Tool");
    else
      strs << ToQStr(t);
  }

  QComboBox toolEdit;
  toolEdit.addItems(strs);
  toolEdit.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  QHBoxLayout executableLayout;

  QLineEdit executableEdit;
  executableEdit.setPlaceholderText(lit("tool"));
  executableEdit.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  executableEdit.setMinimumHeight(20);
  QToolButton executableBrowse;
  executableBrowse.setText(lit("..."));
  executableBrowse.setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

  executableLayout.addWidget(&executableEdit);
  executableLayout.addWidget(&executableBrowse);

  QTextEdit argsEdit;
  argsEdit.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  argsEdit.setMinimumHeight(80);

  strs.clear();

  for(ShaderEncoding enc : values<ShaderEncoding>())
  {
    if(enc == ShaderEncoding::Unknown)
      continue;
    else
      strs << ToQStr(enc);
  }

  QHBoxLayout inputOutputLayout;

  QComboBox inputEdit;
  inputEdit.addItems(strs);

  QComboBox outputEdit;
  outputEdit.addItems(strs);

  inputOutputLayout.addWidget(&inputEdit);
  inputOutputLayout.addWidget(&outputEdit);

  grid.addWidget(&nameEdit, 0, 1, 1, 1);
  grid.addWidget(&toolEdit, 1, 1, 1, 1);
  grid.addLayout(&executableLayout, 2, 1, 1, 1);
  grid.addWidget(&argsEdit, 3, 1, 1, 1);
  grid.addLayout(&inputOutputLayout, 4, 1, 1, 1);

  QDialogButtonBox buttons;
  buttons.setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  grid.addWidget(&buttons, 5, 0, 1, 2);

  QObject::connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  QObject::connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  QObject::connect(&executableBrowse, &QToolButton::clicked, [&]() {
    QString initDir;

    QFileInfo f(executableEdit.text());
    QDir dir = f.dir();
    if(f.isAbsolute() && dir.exists())
    {
      initDir = dir.absolutePath();
    }

    QString filename = RDDialog::getExecutableFileName(this, tr("Choose executable"), initDir);

    if(!filename.isEmpty())
      executableEdit.setText(filename);
  });

  QObject::connect(&toolEdit, OverloadedSlot<int>::of(&QComboBox::currentIndexChanged),
                   [&](int index) {
                     if(index > 0)
                     {
                       KnownShaderTool tool = KnownShaderTool(index);

                       // -1 because we skip ShaderEncoding::Unknown
                       inputEdit.setCurrentIndex(int(ToolInput(tool)) - 1);
                       outputEdit.setCurrentIndex(int(ToolOutput(tool)) - 1);
                       argsEdit.setEnabled(false);
                       inputEdit.setEnabled(false);
                       outputEdit.setEnabled(false);
                     }
                     else
                     {
                       argsEdit.setEnabled(true);
                       inputEdit.setEnabled(true);
                       outputEdit.setEnabled(true);
                     }
                   });

  // -1 because we skip ShaderEncoding::Unknown
  inputEdit.setCurrentIndex(int(tool.input) - 1);
  outputEdit.setCurrentIndex(int(tool.output) - 1);
  executableEdit.setText(tool.executable);
  argsEdit.setText(tool.args);
  nameEdit.setText(tool.name);
  toolEdit.setCurrentIndex(int(tool.tool));

  bool invalid = false;

  do
  {
    RDDialog::show(&dialog);

    // don't validate if they cancelled
    if(dialog.result() != QDialog::Accepted)
      return false;

    tool.tool = KnownShaderTool(toolEdit.currentIndex());
    tool.name = nameEdit.text();
    tool.executable = executableEdit.text();
    tool.args = argsEdit.toPlainText();
    // +1 because we skip ShaderEncoding::Unknown
    tool.input = ShaderEncoding(inputEdit.currentIndex() + 1);
    tool.output = ShaderEncoding(outputEdit.currentIndex() + 1);

    QString message;

    // ensure we don't have an invalid name
    if(tool.name == "Builtin")
    {
      invalid = true;
      message = tr("'Builtin' is a reserved tool name, please select another.");
    }
    else if(tool.name.isEmpty())
    {
      invalid = true;
      message = tr("No tool name specified.");
    }
    else if(tool.executable.isEmpty())
    {
      invalid = true;
      message = tr("No tool executable selected.");
    }
    else if(tool.input == ShaderEncoding::Unknown)
    {
      invalid = true;
      message = tr("Input type cannot be unknown.");
    }
    else if(tool.output == ShaderEncoding::Unknown)
    {
      invalid = true;
      message = tr("Output type cannot be unknown.");
    }
    else if(tool.tool == KnownShaderTool::Unknown &&
            !QString(tool.args).contains(lit("{input_file}")) &&
            !QString(tool.args).contains(lit("{stdin}")))
    {
      invalid = true;
      message = tr("Custom tool arguments must include at least {input_file} or {stdin}.");
    }
    else
    {
      for(int i = 0; i < m_Ctx.Config().ShaderProcessors.count(); i++)
      {
        if(i == existing)
          continue;

        if(tool.name == m_Ctx.Config().ShaderProcessors[i].name)
        {
          invalid = true;
          message = tr("There's already a tool named '%1', please select another.").arg(tool.name);
          break;
        }
      }
    }

    if(invalid)
    {
      RDDialog::critical(this, tr("Invalid parameters specified"), message);
    }
  } while(invalid);

  return true;
}

void SettingsDialog::on_addShaderTool_clicked()
{
  ShaderProcessingTool tool;
  // start with example arguments
  tool.args = lit("--input {input_file} --output {output_file} --mode foo");
  // impossible to pick a single default, but at least show the principle.
  tool.input = ShaderEncoding::HLSL;
  tool.output = ShaderEncoding::SPIRV;

  bool success = editTool(-1, tool);

  if(success)
  {
    m_Ctx.Config().ShaderProcessors.push_back(tool);

    addProcessor(tool);

    m_Ctx.Config().Save();
  }
}

void SettingsDialog::on_editShaderTool_clicked()
{
  int row = -1;

  QModelIndexList selected = ui->shaderTools->selectionModel()->selectedRows();

  if(!selected.isEmpty())
    row = selected[0].row();

  if(row < 0 || row >= m_Ctx.Config().ShaderProcessors.count())
    return;

  ShaderProcessingTool tool = m_Ctx.Config().ShaderProcessors[row];

  bool success = editTool(row, tool);

  if(success)
  {
    ui->shaderTools->setItem(row, 0, new QTableWidgetItem(tool.name));
    ui->shaderTools->setItem(
        row, 1, new QTableWidgetItem(
                    QFormatStr("%1 -> %2").arg(ToQStr(tool.input)).arg(ToQStr(tool.output))));
    m_Ctx.Config().ShaderProcessors[row] = tool;
    m_Ctx.Config().Save();
  }
}

void SettingsDialog::on_deleteShaderTool_clicked()
{
  int row = -1;

  QModelIndexList selected = ui->shaderTools->selectionModel()->selectedRows();

  if(!selected.isEmpty())
    row = selected[0].row();

  if(row < 0 || row >= m_Ctx.Config().ShaderProcessors.count())
    return;

  const ShaderProcessingTool &tool = m_Ctx.Config().ShaderProcessors[row];

  QMessageBox::StandardButton res = RDDialog::question(
      this, tr("Are you sure?"), tr("Are you sure you want to delete '%1'?").arg(tool.name),
      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

  if(res == QMessageBox::Yes)
  {
    ui->shaderTools->removeRow(row);
    m_Ctx.Config().ShaderProcessors.erase(row);

    m_Ctx.Config().Save();
  }
}

void SettingsDialog::on_shaderTools_itemSelectionChanged()
{
  ui->deleteShaderTool->setEnabled(!ui->shaderTools->selectionModel()->selectedIndexes().empty());
  ui->editShaderTool->setEnabled(ui->deleteShaderTool->isEnabled());
}

void SettingsDialog::on_shaderTools_keyPress(QKeyEvent *event)
{
  if(event->key() == Qt::Key_Delete)
  {
    ui->deleteShaderTool->click();
  }
  if(event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
  {
    ui->editShaderTool->click();
  }
}

void SettingsDialog::on_shaderTools_itemDoubleClicked(QTableWidgetItem *item)
{
  ui->editShaderTool->click();
}

void SettingsDialog::shaderTools_rowMoved(int logicalIndex, int oldVisualIndex, int newVisualIndex)
{
  if(oldVisualIndex < 0 || oldVisualIndex >= m_Ctx.Config().ShaderProcessors.count() ||
     newVisualIndex < 0 || newVisualIndex >= m_Ctx.Config().ShaderProcessors.count())
    return;

  ShaderProcessingTool tool = m_Ctx.Config().ShaderProcessors.at(oldVisualIndex);
  m_Ctx.Config().ShaderProcessors.erase(oldVisualIndex);
  m_Ctx.Config().ShaderProcessors.insert(newVisualIndex, tool);

  m_Ctx.Config().Save();
}

// event browser
void SettingsDialog::on_EventBrowser_TimeUnit_currentIndexChanged(int index)
{
  if(m_Init)
    return;

  m_Ctx.Config().EventBrowser_TimeUnit = (TimeUnit)qMax(0, ui->EventBrowser_TimeUnit->currentIndex());

  if(m_Ctx.HasEventBrowser())
    m_Ctx.GetEventBrowser()->UpdateDurationColumn();

  m_Ctx.Config().Save();
}

void SettingsDialog::on_EventBrowser_AddFake_toggled(bool checked)
{
  m_Ctx.Config().EventBrowser_AddFake = ui->EventBrowser_AddFake->isChecked();

  m_Ctx.Config().Save();
}

void SettingsDialog::on_EventBrowser_HideEmpty_toggled(bool checked)
{
  m_Ctx.Config().EventBrowser_HideEmpty = ui->EventBrowser_HideEmpty->isChecked();

  m_Ctx.Config().Save();
}

void SettingsDialog::on_EventBrowser_HideAPICalls_toggled(bool checked)
{
  m_Ctx.Config().EventBrowser_HideAPICalls = ui->EventBrowser_HideAPICalls->isChecked();

  m_Ctx.Config().Save();
}

void SettingsDialog::on_EventBrowser_ApplyColors_toggled(bool checked)
{
  m_Ctx.Config().EventBrowser_ApplyColors = ui->EventBrowser_ApplyColors->isChecked();

  m_Ctx.Config().Save();
}

void SettingsDialog::on_EventBrowser_ColorEventRow_toggled(bool checked)
{
  m_Ctx.Config().EventBrowser_ColorEventRow = ui->EventBrowser_ColorEventRow->isChecked();

  m_Ctx.Config().Save();
}

void SettingsDialog::on_Comments_ShowOnLoad_toggled(bool checked)
{
  m_Ctx.Config().Comments_ShowOnLoad = ui->Comments_ShowOnLoad->isChecked();

  m_Ctx.Config().Save();
}

// android
void SettingsDialog::on_browseTempCaptureDirectory_clicked()
{
  QString dir = RDDialog::getExistingDirectory(this, tr("Choose directory for temporary captures"),
                                               m_Ctx.Config().TemporaryCaptureDirectory);

  if(!dir.isEmpty())
  {
    m_Ctx.Config().TemporaryCaptureDirectory = dir;
    ui->tempDirectory->setText(dir);
  }

  m_Ctx.Config().Save();
}

void SettingsDialog::on_browseAndroidSDKPath_clicked()
{
  QString adb = RDDialog::getExistingDirectory(
      this, tr("Locate SDK root folder (containing build-tools, platform-tools)"),
      QFileInfo(m_Ctx.Config().Android_SDKPath).absoluteDir().path());

  if(!adb.isEmpty())
  {
    ui->Android_SDKPath->setText(adb);
    m_Ctx.Config().Android_SDKPath = adb;
  }

  m_Ctx.Config().Save();
}

void SettingsDialog::on_Android_SDKPath_textEdited(const QString &adb)
{
  if(QFileInfo::exists(adb) || adb.isEmpty())
    m_Ctx.Config().Android_SDKPath = adb;

  m_Ctx.Config().Save();
}

void SettingsDialog::on_browseJDKPath_clicked()
{
  QString adb =
      RDDialog::getExistingDirectory(this, tr("Locate JDK root folder (containing bin, jre, lib)"),
                                     QFileInfo(m_Ctx.Config().Android_JDKPath).absoluteDir().path());

  if(!adb.isEmpty())
  {
    ui->Android_JDKPath->setText(adb);
    m_Ctx.Config().Android_JDKPath = adb;
  }

  m_Ctx.Config().Save();
}

void SettingsDialog::on_Android_JDKPath_textEdited(const QString &adb)
{
  if(QFileInfo::exists(adb) || adb.isEmpty())
    m_Ctx.Config().Android_JDKPath = adb;

  m_Ctx.Config().Save();
}

void SettingsDialog::on_Android_MaxConnectTimeout_valueChanged(double timeout)
{
  m_Ctx.Config().Android_MaxConnectTimeout = ui->Android_MaxConnectTimeout->value();

  m_Ctx.Config().Save();
}

void SettingsDialog::on_UIStyle_currentIndexChanged(int index)
{
  if(index < 0 || index >= StyleData::numAvailable)
    return;

  // don't do anything until the dialog is initialised and visible
  if(!isVisible())
    return;

  QString oldStyle = m_Ctx.Config().UIStyle;
  QString newStyle = StyleData::availStyles[index].styleID;

  if(oldStyle == newStyle)
    return;

  QMessageBox::StandardButton ret = RDDialog::question(
      this, tr("Switch to new theme?"),
      tr("Would you like to switch to the new theme now?<br><br>Some parts of a theme might "
         "require a full application restart to properly apply."),
      RDDialog::YesNoCancel, QMessageBox::Yes);

  if(ret == QMessageBox::Cancel)
  {
    // change the index back. Since we haven't changed the style yet, this will early out above
    // instead of recursing.

    for(int i = 0; i < StyleData::numAvailable; i++)
    {
      if(StyleData::availStyles[i].styleID == oldStyle)
      {
        ui->UIStyle->setCurrentIndex(i);
        break;
      }
    }

    return;
  }

  // set the style but don't change anything unless the user selected yes.
  m_Ctx.Config().UIStyle = newStyle;

  if(ret == QMessageBox::Yes)
  {
    m_Ctx.Config().SetStyle();
  }

  m_Ctx.Config().Save();
}