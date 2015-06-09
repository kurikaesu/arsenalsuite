
/*
 *
 * Copyright 2003 Blur Studio Inc.
 *
 * This file is part of libstone.
 *
 * libstone is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libstone is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libstone; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id$
 */

#include <qstring.h>
#include <qstringlist.h>

#include "gvgraph.h"

/*! Dot uses a 72 DPI value for converting it's position coordinates from points to pixels
    while we display at 96 DPI on most operating systems. */
const qreal GVGraph::DotDefaultDPI=72.0;

GVGraph::GVGraph(QString name, QFont font, qreal node_size) :
        _context(gvContext()),
        _graph(_agopen(name, AGDIGRAPH)) // Strict directed graph, see libgraph doc
{
    //Set graph attributes
    _agset(_graph, "overlap", "prism");
    _agset(_graph, "splines", "true");
    _agset(_graph, "pad", "0,2");
    _agset(_graph, "dpi", "96,0");
    _agset(_graph, "nodesep", "0,4");

    //Set default attributes for the future nodes
    _agnodeattr(_graph, "fixedsize", "true");
    _agnodeattr(_graph, "regular", "false");
    _agnodeattr(_graph, "shape", "ellipse");

    //Divide the wanted width by the DPI to get the value in points
    //QString nodePtsWidth = QString::number(node_size / _agget(_graph, "dpi", "96,0").toDouble());
    //QString nodePtsHeight = QString::number((node_size*0.8) / _agget(_graph, "dpi", "96,0").toDouble());

    //GV uses , instead of . for the separator in floats
    //_agnodeattr(_graph, "width", nodePtsWidth.replace(".",","));
    //_agnodeattr(_graph, "height", nodePtsHeight);

    setFont(font);
}

GVGraph::~GVGraph()
{
    gvFreeLayout(_context, _graph);
    agclose(_graph);
    gvFreeContext(_context);
}

/***
We will now see how to add and remove nodes. The node removal function takes care of removing edges as well, so you may want to come back to it after we introduced edges (eventhough the code is pretty straightforward).
***/

void GVGraph::addNode(const QString& name)
{
    if(_nodes.contains(name))
        return;

    _nodes.insert(name, _agnode(_graph, name));
}

void GVGraph::addNodes(const QStringList& names)
{
    for(int i=0; i<names.size(); ++i)
        addNode(names.at(i));
}

void GVGraph::removeNode(const QString& name)
{
    if(_nodes.contains(name))
    {
        agdelete(_graph, _nodes[name]);
        _nodes.remove(name);

        QList<QPair<QString, QString> > keys = _edges.keys();
        for(int i=0; i<keys.size(); ++i)
            if(keys.at(i).first==name || keys.at(i).second==name)
                removeEdge(keys.at(i));
    }
}

void GVGraph::clearNodes()
{
    QList<QString> keys=_nodes.keys();

    for(int i=0; i<keys.size(); ++i)
        removeNode(keys.at(i));
}

bool GVGraph::setNodeAttr(const QString& name, const QString & attr, const QString & value)
{
    if( !_nodes.contains(name) ) return false;
    _agset(_nodes[name], attr, value);
    return true;
}

bool GVGraph::setEdgeAttr(const QPair<QString, QString> & name, const QString & attr, const QString & value)
{
    if( !_edges.contains(name) ) return false;
    _agset(_edges[name], attr, value);
    return true;
}

/***
The special function setRootNode() allows telling the dot layout algorithm which node we want to use as a root for generating the graph (as dot usually displays tree-like graphs, it can start drawing them from a particular node).
***/
void GVGraph::setRootNode(const QString& name)
{
    if(_nodes.contains(name))
        _agset(_graph, "root", name);
}

/***
And now the functions for drawing edges, which are pretty straightforward too:
***/
void GVGraph::addEdge(const QString &source, const QString &target)
{
    if(_nodes.contains(source) && _nodes.contains(target))
    {
        QPair<QString, QString> key(source, target);
        if(!_edges.contains(key))
            _edges.insert(key, agedge(_graph, _nodes[source], _nodes[target]));
    }
}

void GVGraph::removeEdge(const QString &source, const QString &target)
{
    removeEdge(QPair<QString, QString>(source, target));
}

void GVGraph::removeEdge(const QPair<QString, QString>& key)
{
    if(_edges.contains(key))
    {
        agdelete(_graph, _edges[key]);
        _edges.remove(key);
    }
}

/***
The setFont() method allows you to tell Graphviz which font you want to use for the rendering. You can pass the constructor your application's current font and write an event handler for when the font changes in your application, that will call this function. Note that I'm not sure it updates the font for existing nodes and edges (in which case you would have to call _agset() on all of them).
***/
void GVGraph::setFont(QFont font)
{
    _font=font;

    _agset(_graph, "fontname", font.family());
    _agset(_graph, "fontsize", QString("%1").arg(font.pointSizeF()));

    _agnodeattr(_graph, "fontname", font.family());
    _agnodeattr(_graph, "fontsize", QString("%1").arg(font.pointSizeF()));

    _agedgeattr(_graph, "fontname", font.family());
    _agedgeattr(_graph, "fontsize", QString("%1").arg(font.pointSizeF()));
}

/***
Surprisingly, this is the very easy part of the job. Graphviz already has several implementations of common graph drawing algorithms, used for different kinds of graphs. In Graphviz, they are called layout, and the most known one is dot. It also is the one I'm using, but you may write additional methods in your class for calling different kinds of layouts – just be aware that some layouts require extra attributes, so you may also want to modify other parts of the class depending on what you need.
***/
void GVGraph::applyLayout(const QString & _layoutName)
{
    gvFreeLayout(_context, _graph);
    _gvLayout(_context, _graph, _layoutName);
}

void GVGraph::render(const QString & fileName)
{
    _gvRender(_context, _graph, fileName);
}

/***
Once the layout has been called, many more attributes are set in the graph, nodes and edges's data field. Trying to access this information in the objects before would have lead to unitialised values and null pointers, so there are things that we can do only after having applied a layout. Among these things, we can now retrieve the exact size of the graph in points, and give this information to our QGraphicsView's setSceneRect() method, which defines the area of the QGraphicsScene that the user can see.
***/
QRectF GVGraph::boundingRect() const
{
    qreal dpi=_agget(_graph, "dpi", "96,0").toDouble();
    return QRectF(_graph->u.bb.LL.x*(dpi/DotDefaultDPI), _graph->u.bb.LL.y*(dpi/DotDefaultDPI),
                  _graph->u.bb.UR.x*(dpi/DotDefaultDPI), _graph->u.bb.UR.y*(dpi/DotDefaultDPI));
}

QList<GVNode> GVGraph::nodes() const
{
    QList<GVNode> list;
    qreal dpi=_agget(_graph, "dpi", "96,0").toDouble();

    for(QMap<QString, Agnode_t*>::const_iterator it=_nodes.begin(); it!=_nodes.end();++it)
    {
        Agnode_t *node=it.value();
        GVNode object;

        //Set the name of the node
        object.name=node->name;

        //Fetch the X coordinate, apply the DPI conversion rate (actual DPI / 72, used by dot)
        qreal x=node->u.coord.x*(dpi/DotDefaultDPI);

        //Translate the Y coordinate from bottom-left to top-left corner
        qreal y=(_graph->u.bb.UR.y - node->u.coord.y)*(dpi/DotDefaultDPI);
        object.centerPos=QPoint(x, y);

        //Transform the width and height from inches to pixels
        object.height=node->u.height*dpi;
        object.width=node->u.width*dpi;

        list << object;
    }

    return list;
}

QList<GVEdge> GVGraph::edges() const
{
    QList<GVEdge> list;
    qreal dpi=_agget(_graph, "dpi", "96,0").toDouble();

    for(QMap<QPair<QString, QString>, Agedge_t*>::const_iterator it=_edges.begin();
        it!=_edges.end();++it)
    {
        Agedge_t *edge=it.value();
        GVEdge object;

        //Fill the source and target node names
        object.source=edge->tail->name;
        object.target=edge->head->name;

        //Calculate the path from the spline (only one spline, as the graph is strict. If it
        //wasn't, we would have to iterate over the first list too)
        //Calculate the path from the spline (only one as the graph is strict)
        if((edge->u.spl->list!=0) && (edge->u.spl->list->size%3 == 1))
        {
            //If there is a starting point, draw a line from it to the first curve point
            if(edge->u.spl->list->sflag)
            {
                object.path.moveTo(edge->u.spl->list->sp.x*(dpi/DotDefaultDPI),
                             (_graph->u.bb.UR.y - edge->u.spl->list->sp.y)*(dpi/DotDefaultDPI));
                object.path.lineTo(edge->u.spl->list->list[0].x*(dpi/DotDefaultDPI),
                        (_graph->u.bb.UR.y - edge->u.spl->list->list[0].y)*(dpi/DotDefaultDPI));
            }
            else
                object.path.moveTo(edge->u.spl->list->list[0].x*(dpi/DotDefaultDPI),
                        (_graph->u.bb.UR.y - edge->u.spl->list->list[0].y)*(dpi/DotDefaultDPI));

            //Loop over the curve points
            for(int i=1; i<edge->u.spl->list->size; i+=3)
                object.path.cubicTo(edge->u.spl->list->list[i].x*(dpi/DotDefaultDPI), 
                      (_graph->u.bb.UR.y - edge->u.spl->list->list[i].y)*(dpi/DotDefaultDPI),
                      edge->u.spl->list->list[i+1].x*(dpi/DotDefaultDPI),
                      (_graph->u.bb.UR.y - edge->u.spl->list->list[i+1].y)*(dpi/DotDefaultDPI),
                      edge->u.spl->list->list[i+2].x*(dpi/DotDefaultDPI),
                      (_graph->u.bb.UR.y - edge->u.spl->list->list[i+2].y)*(dpi/DotDefaultDPI));

            //If there is an ending point, draw a line to it
            if(edge->u.spl->list->eflag)
                object.path.lineTo(edge->u.spl->list->ep.x*(dpi/DotDefaultDPI),
                             (_graph->u.bb.UR.y - edge->u.spl->list->ep.y)*(dpi/DotDefaultDPI));
        }

        list << object;
    }

    return list;
}



