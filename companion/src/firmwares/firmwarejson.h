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

#pragma once

#include "eeprominterface.h"

#include <QtCore>

class QJsonDocument;
class QJsonObject;

class FirmwareJson
{
  Q_DECLARE_TR_FUNCTIONS(FirmwareJson)

  public:

    typedef QList<QString> OptionsGroup;
    typedef QList<OptionsGroup> OptionsList;

    struct FirmwareDefn {
      QString id          = "";
      QString name        = "";
      QString board       = "";
      QString downloadId  = "";
      QString simulatorId = "";
      QString hwdefnId    = ""; // if board does not match hwdef
      bool    categories  = true;
      bool    color       = true;
      int     gvars       = 8;

      OptionsList options;

      FirmwareDefn() = default;
    };

    explicit FirmwareJson();
    virtual ~FirmwareJson();

    void registerAllFirmwareDefns();

  private:
    // tooltip translation cannot be performed at runtime
    // so convert and load mapping at compile time
    // key   name
    // value tooltip
    typedef QMap<QString, QString> OptionTooltip;
    static const OptionTooltip availableOptions;

    const QVariant getValue(const QJsonObject & obj, const QString & name, const QVariant & dflt) const;
    bool load(QJsonDocument * doc, const QString & path);
    bool loadDefinition(FirmwareDefn & defn, QString & path);
    void registerFirmwareDefn(FirmwareDefn & defn);
    bool isOptionDuplicate(const OptionsList & options, const QString & val);
};