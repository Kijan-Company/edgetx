/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "firmwarejson.h"

#include <QByteArray>
#include <QDirIterator>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>

constexpr char FWDEFNSDIR[] { ":/fwdefs" };

// static
const FirmwareJson::OptionTooltip FirmwareJson::availableOptions = {
  { "opt1", QT_TRANSLATE_NOOP("Firmware", "This is option 1") },
  { "opt2", QT_TRANSLATE_NOOP("Firmware", "This is option 2") },
  { "opt3", QT_TRANSLATE_NOOP("Firmware", "This is option 3") },
  { "opt4", QT_TRANSLATE_NOOP("Firmware", "This is option 4") },
  { "opt5", QT_TRANSLATE_NOOP("Firmware", "This is option 5") }
};

FirmwareJson::FirmwareJson()
{

}

FirmwareJson::~FirmwareJson()
{
}

void FirmwareJson::registerFirmwareDefn(FirmwareDefn & defn)
{
  qDebug() << "registering id:" << defn.id
           << "name:" << defn.name
           << "board:" << defn.board
           << "downloadId" << defn.downloadId
           << "simulatorId" << defn.simulatorId
           << "hwdefnId" << defn.hwdefnId
           << "options:" << defn.options
           << "categories:" << defn.categories
           << "color:" << defn.color
           << "gvars:" << defn.gvars;

  // need to check for duplicate id

  // for board or hwdef look up hwdef file in qrc

}

void FirmwareJson::registerAllFirmwareDefns()
{
  QStringList filters = { "*.json" };

  QDirIterator it(QString("%1/").arg(FWDEFNSDIR), filters, QDir::Files);

  while (it.hasNext()) {
    QString path = it.next();
    qDebug() << "found:" << path;
    FirmwareDefn defn;
    QJsonDocument *doc = new QJsonDocument();

    if (load(doc, path)) {
      QJsonObject obj = doc->object();
      // ignore intermediate definitions
      if (!getValue(obj, "hidden", false).toBool()) {
        QString id = getValue(obj, "id", "unknown").toString();
        qDebug() << "loading definition for id:" << id;

        if (loadDefinition(defn, path)) {
          registerFirmwareDefn(defn);
        } else {
          qCritical() << "not registering id:" << id;
        }
      } else {
        qDebug() << "ignoring" << path;
      }
    }

    delete doc;
  }
}

bool FirmwareJson::loadDefinition(FirmwareDefn & defn, QString & path)
{
  qDebug() << "path" << path;
  bool success = true;
  QJsonDocument *doc = new QJsonDocument();
  QJsonObject o;
  QStringList depends;

  if (load(doc, path)) {
    if (doc->isObject()) {
      o = doc->object();

      if (!o.value("basedOn").isUndefined() && o.value("basedOn").isArray()) {
        QJsonArray a = o.value("basedOn").toArray();

        for (QJsonArray::const_iterator it = a.constBegin(); it != a.constEnd(); ++it) {
          if ((*it).isString()) {
            QString path = QString("%1/%2.json").arg(FWDEFNSDIR).arg((*it).toString());

            if (!depends.contains(path)) {
              depends.append(path);

              if (!loadDefinition(defn, path))
                success = false;
            } else {
              qCritical() << "ERROR: circular dependency chain detected";
              success = false;
            }
          }
        }
      }
    }
  } else {
    success = false;
  }

  if (!success) {
    qDebug() << "unsuccessful";
    delete doc;
    return false;
  }

  qDebug() << "loading values from:" << path;

  for (QJsonObject::const_iterator it = o.constBegin(); it != o.constEnd(); ++it) {
    qDebug() << "key:" << it.key() << "value:" << it.value();

    if (it.key() == "hidden" || it.key() == "basedOn")
      continue;

    if (it.key() == "id" && it.value().isString())
      defn.id = it.value().toString();
    else if (it.key() == "name" && it.value().isString())
      defn.name = it.value().toString();
    else if (it.key() == "board" && it.value().isString())
      defn.board = it.value().toString();
    else if (it.key() == "downloadId" && it.value().isString())
      defn.downloadId = it.value().toString();
    else if (it.key() == "simulatorId" && it.value().isString())
      defn.simulatorId = it.value().toString();
    else if (it.key() == "hwdefnId" && it.value().isString())
      defn.hwdefnId = it.value().toString();
    else if (it.key() == "categories" && it.value().isBool())
      defn.categories = it.value().toBool();
    else if (it.key() == "color" && it.value().isBool())
      defn.color = it.value().toBool();
    else if (it.key() == "gvars" && it.value().isDouble())
      defn.gvars = (it.value().toInt() <= CPN_MAX_GVARS ? it.value().toInt() : defn.gvars);
    else if (it.key() == "options" && it.value().isArray()) {
      QJsonArray arrOptions = it->toArray();

      for (QJsonArray::const_iterator itOptions = arrOptions.constBegin(); itOptions != arrOptions.constEnd(); ++itOptions) {
        qDebug() << "value:" << (*itOptions);

        if ((*itOptions).isArray()) {
          OptionsGroup grp;
          QJsonArray arrOptGrp = itOptions->toArray();

          for (QJsonArray::const_iterator itOptGrp = arrOptGrp.constBegin(); itOptGrp != arrOptGrp.constEnd(); ++itOptGrp) {
            bool isValid = availableOptions.contains((*itOptGrp).toString());
            bool isDuplicate = (grp.contains((*itOptGrp).toString()) ||
                                isOptionDuplicate(defn.options, (*itOptGrp).toString()));

            qDebug() << "value:" << (*itOptGrp) << "valid:" << isValid << "duplicate:" << isDuplicate;

            if (isValid && !isDuplicate) {
              grp.append((*itOptGrp).toString());
            }
          }

          defn.options.append(grp);
        } else if ((*itOptions).isString()) {
          bool isValid = availableOptions.contains((*itOptions).toString());
          bool isDuplicate = isOptionDuplicate(defn.options, (*itOptions).toString());

          qDebug() << "value:" << (*itOptions) << "valid:" << isValid << "duplicate:" << isDuplicate;

          if (isValid && !isDuplicate) {
            defn.options.append( { (*itOptions).toString() });
          }
        }
      }
    }
    else
      qWarning() << "Warning: No rule to process - name:" << it.key() << "value:" << it.value();
  }

  delete doc;
  return true;
}

bool FirmwareJson::isOptionDuplicate(const OptionsList & options, const QString & val)
{
  qDebug() << val;

  for (OptionsList::const_iterator it = options.cbegin(); it != options.cend(); ++it) {
    for (OptionsGroup::const_iterator itg = it->cbegin(); itg != it->cend(); ++itg) {
      //qDebug() << *itg;
      if (val == *itg)
        return true;
    }
  }

  return false;
}

const QVariant FirmwareJson::getValue(const QJsonObject & obj, const QString & name, const QVariant & dflt) const
{
  if (!obj.value(name).isUndefined())
    return obj.value(name);
  else
    return dflt;
}

bool FirmwareJson::load(QJsonDocument * doc, const QString & filename)
{
  QFile file(filename);

  if (!file.open(QIODevice::ReadOnly)) {
    QMessageBox::critical(nullptr, tr("Load Firmware Definition"),
                          tr("Error: Unable to open file %1").arg(file.fileName()));
    return false;
  }

  QByteArray *buffer = new QByteArray();
  *buffer = file.readAll();
  file.close();

  if (buffer->isEmpty()) {
    QMessageBox::critical(nullptr, tr("Load Firmware Definition"),
                          tr("Error: Unable to read file %1").arg(file.fileName()));
    return false;
  }

  QJsonParseError res;
  *doc = QJsonDocument::fromJson(*buffer, &res);
  delete buffer;

  if (res.error || doc->isNull() || !doc->isObject()) {
    QMessageBox::critical(nullptr, tr("Load Firmware Definition"),
      tr("Error: %1 is not a valid json formatted file.\nError code: %2\nError description: %3")
          .arg(file.fileName()).arg(res.error).arg(res.errorString()));
    *doc = QJsonDocument();
    return false;
  }

  return true;
}




