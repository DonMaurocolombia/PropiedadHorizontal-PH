// SPDX-FileCopyrightText: 2026 Oscar Mauricio Bonil P.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/*
   Administrador - PH: Administrador PH busca facilitar y organizar la 
   administración de copropiedades mediante una plataforma que centralice 
   la información, los documentos y los procesos administrativos, bajo la 
   Licencia GPL-3.0-or-later.
   
   Copyright (C) 2026 Oscar Mauricio Bonil P.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/


#pragma once

#include <QProxyStyle>
#include <QStyleOptionTab>
#include <QTabBar>
#include <QPainter>
#include <QColor>


class VerticalTabStyle : public QProxyStyle {
public:
    explicit VerticalTabStyle(QStyle* style = nullptr) : QProxyStyle(style) {}

    // Transponer el tamaño: ancho ↔ alto
    // (para que el tab sea lo suficientemente ancho para texto horizontal)
    QSize sizeFromContents(ContentsType type, const QStyleOption* option,
                           const QSize& size, const QWidget* widget) const override {
        QSize s = QProxyStyle::sizeFromContents(type, option, size, widget);
        if (type == QStyle::CT_TabBarTab)
            s.transpose();
        return s;
    }

    // Forzar el texto a horizontal dibujándolo como si fuera RoundedNorth
    void drawControl(ControlElement element, const QStyleOption* option,
                     QPainter* painter, const QWidget* widget) const override {
        if (element == CE_TabBarTabLabel) {
            if (const QStyleOptionTab* tab =
                    qstyleoption_cast<const QStyleOptionTab*>(option)) {
                // Dibuja el texto a la izquierda con color fijo
                painter->save();
                painter->setPen(QColor("black"));
                QRect r = tab->rect.adjusted(12, 0, -6, 0);  // margen izquierdo
                painter->drawText(r, Qt::AlignLeft | Qt::AlignVCenter, tab->text);
                painter->restore();

                return;
            }
        }

        QProxyStyle::drawControl(element, option, painter, widget);
    }
};
