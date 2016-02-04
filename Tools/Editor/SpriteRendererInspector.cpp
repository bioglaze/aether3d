#include "SpriteRendererInspector.hpp"
#include <QLabel>
#include <QBoxLayout>
#include <QWidget>
#include <QString>
#include <QTableWidgetItem>

void SpriteRendererInspector::Init( QWidget* mainWindow )
{
    table = new QTableWidget( 1, 1 );
    table->setItem( 0, 0, new QTableWidgetItem() );
    table->setHorizontalHeaderLabels( QString("").split(";") );
    table->setVerticalHeaderLabels( QString("Sprite").split(";") );

    table->setItem( 0, 0, new QTableWidgetItem() );

    QBoxLayout* inspectorLayout = new QBoxLayout( QBoxLayout::TopToBottom );
    inspectorLayout->setContentsMargins( 1, 1, 1, 1 );
    inspectorLayout->addWidget( new QLabel( "Sprite Renderer" ) );
    inspectorLayout->addWidget( table );

    root = new QWidget();
    root->setLayout( inspectorLayout );
}
