#include "preferenceform.h"
#include "ui_preference.h"

PreferenceForm::PreferenceForm(QWidget* parent)
    : QWidget(parent), ui(new Ui::preferenceForm) {
  ui->setupUi(this);
  setWindowTitle(tr("Preference"));
}

PreferenceForm::~PreferenceForm() {
  delete ui;
}
