
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

#ifndef GV_GRAPH_H
#define GV_GRAPH_H

#include "graphviz/types.h"
#include "graphviz/graph.h"
#include "graphviz/gvc.h"

#include <QString>
#include <QMap>
#include <QFont>
#include <QPoint>
#include <QPainterPath>
#include <QPair>

/// The agopen method for opening a graph
static inline Agraph_t* _agopen(QString name, int kind)
{
    return agopen(const_cast<char *>(qPrintable(name)), kind);
}

/// Add an alternative value parameter to the method for getting an object's attribute
static inline QString _agget(void *object, QString attr, QString alt=QString())
{
    QString str=agget(object, const_cast<char *>(qPrintable(attr)));

    if(str==QString())
        return alt;
    else
        return str;
}

/// Directly use agsafeset which always works, contrarily to agset
static inline int _agset(void *object, QString attr, QString value)
{
    return agsafeset(object, const_cast<char *>(qPrintable(attr)),
                     const_cast<char *>(qPrintable(value)),
                     const_cast<char *>(qPrintable(value)));
}

static inline Agsym_t* _agnodeattr(Agraph_t *object, QString attr, QString alt=QString())
{
    return agnodeattr(object,
                     const_cast<char *>(qPrintable(attr)),
                     const_cast<char *>(qPrintable(alt )));
}

static inline Agsym_t* _agedgeattr(Agraph_t *object, QString attr, QString alt=QString())
{
    return agedgeattr(object,
                     const_cast<char *>(qPrintable(attr)),
                     const_cast<char *>(qPrintable(alt)));
}

static inline Agnode_t* _agnode(Agraph_t *object, QString node)
{
    return agnode(object,
                     const_cast<char *>(qPrintable(node)));
}

static inline int _gvLayout(GVC_t *gvc, graph_t *g, QString engine)
{
    return gvLayout(gvc, g,
                     const_cast<char *>(qPrintable(engine)));
}

static inline int _gvRender(GVC_t *gvc, Agraph_t *g, QString filename)
{
    return gvRenderFilename(gvc, g, "png",
                     const_cast<char *>(qPrintable(filename)));
}

/// A struct containing the information for a GVGraph's node
struct GVNode
{
    /// The unique identifier of the node in the graph
    QString name;

    /// The position of the center point of the node from the top-left corner
    QPoint centerPos;

    /// The size of the node in pixels
    double height, width;
};


/// A struct containing the information for a GVGraph's edge
struct GVEdge
{
    /// The source and target nodes of the edge
    QString source;
    QString target;

    /// Path of the edge's line
    QPainterPath path;
};

/// An object containing a libgraph graph and its associated nodes and edges
class GVGraph
{
public:
    /// Default DPI value used by dot (which uses points instead of pixels for coordinates)
    static const qreal DotDefaultDPI;

    /*!
     * \brief Construct a Graphviz graph object
     * \param name The name of the graph, must be unique in the application
     * \param font The font to use for the graph
     * \param node_size The size in pixels of each node
     */
    GVGraph(QString name, QFont font=QFont(), qreal node_size=72);
    ~GVGraph();

    void setRootNode(const QString& name);

    void applyLayout(const QString & );
    void render(const QString & fileName);

    QRectF boundingRect() const;

    /// Add and remove nodes
    void addNode(const QString& name);
    void addNodes(const QStringList& names);
    void removeNode(const QString& name);
    void clearNodes();

    bool setNodeAttr(const QString& source, const QString& attr, const QString & value);
    bool setEdgeAttr(const QPair<QString, QString> &, const QString& attr, const QString & value);

    /// Add and remove edges
    void addEdge(const QString& source, const QString& target);
    void removeEdge(const QString& source, const QString& target);
    void removeEdge(const QPair<QString, QString> &);

    /// Set the font to use in all the labels
    void setFont(QFont font);

    QList<GVNode> nodes() const;
    QList<GVEdge> edges() const;

private:
    GVC_t *_context;
    Agraph_t *_graph;
    QFont _font;
    QMap<QString, Agnode_t*> _nodes;
    QMap<QPair<QString, QString>, Agedge_t*> _edges;
};

#endif // GL_UTIL_H

