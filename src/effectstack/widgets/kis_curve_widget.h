/*
 *  Copyright (c) 2005 Casper Boemann <cbr@boemann.dk>
 *  Copyright (c) 2009 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef KIS_CURVE_WIDGET_H
#define KIS_CURVE_WIDGET_H

// Qt includes.

#include <QWidget>

#include "effectstack/widgets/curves/abstractcurvewidget.h"
#include "kis_cubic_curve.h"

class QEvent;
class QKeyEvent;
class QMouseEvent;
class QObject;
class QPaintEvent;
class QPixmap;
class QResizeEvent;

class QSpinBox;

/**
 * KisCurveWidget is a widget that shows a single curve that can be edited
 * by the user. The user can grab the curve and move it; this creates
 * a new control point. Control points can be deleted by selecting a point
 * and pressing the delete key.
 *
 * (From: http://techbase.kde.org/Projects/Widgets_and_Classes#KisCurveWidget)
 * KisCurveWidget allows editing of spline based y=f(x) curves. Handy for cases
 * where you want the user to control such things as tablet pressure
 * response, color transformations, acceleration by time, aeroplane lift
 *by angle of attack.
 */
class KisCurveWidget : public AbstractCurveWidget<KisCubicCurve>
{
    Q_OBJECT

public:

    /**
     * Create a new curve widget with a default curve, that is a straight
     * line from bottom-left to top-right.
     */
    explicit KisCurveWidget(QWidget *parent = Q_NULLPTR);

    virtual ~KisCurveWidget();

    /**
     * Reset the curve to the default shape
     */
    void reset(void);


    QSize sizeHint() const Q_DECL_OVERRIDE;


protected slots:
    void inOutChanged(int);

protected:

    void keyPressEvent(QKeyEvent *) Q_DECL_OVERRIDE;
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent *) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *e) Q_DECL_OVERRIDE;
public:


    /**
     * Connect/disconnect external spinboxes to the curve
     * @min/@max - is the range for their values
     */
    void setupInOutControls(QSpinBox *in, QSpinBox *out, int min, int max);
    void dropInOutControls();

    /**
     * Handy function that creates new point in the middle
     * of the curve and sets focus on the m_intIn field,
     * so the user can move this point anywhere in a moment
     */
    void addPointInTheMiddle();

    void setCurve(KisCubicCurve&& curve);

    QList<QPointF> getPoints() const override;

private:
    double io2sp(int x) const;
    int sp2io(double x) const;
    bool jumpOverExistingPoints(QPointF &pt, int skipIndex);
    void syncIOControls();
    int nearestPointInRange(QPointF pt, int wWidth, int wHeight) const;

    /* Dragging variables */
    double m_grabOffsetX;
    double m_grabOffsetY;
    double m_grabOriginalX;
    double m_grabOriginalY;
    QPointF m_draggedAwayPoint;
    int m_draggedAwayPointIndex;

    bool m_readOnlyMode;
    bool m_guideVisible;
    QColor m_colorGuide;


    /* In/Out controls */
    QSpinBox *m_intIn;
    QSpinBox *m_intOut;

    /* Working range of them */
    int m_inOutMin;
    int m_inOutMax;

};

#endif /* KIS_CURVE_WIDGET_H */
