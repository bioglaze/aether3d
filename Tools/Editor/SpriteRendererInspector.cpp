#include "SpriteRendererInspector.hpp"
#include <QLabel>
#include <QBoxLayout>
#include <QWidget>
#include <QString>
#include <QTableWidgetItem>
#include <QPushButton>

void SpriteRendererInspector::Init( QWidget* mainWindow )
{
    table = new QTableWidget( 1, 1 );
    table->setItem( 0, 0, new QTableWidgetItem() );
    table->setHorizontalHeaderLabels( QString("").split(";") );
    table->setVerticalHeaderLabels( QString("Sprite").split(";") );

    table->setItem( 0, 0, new QTableWidgetItem() );

    removeButton = new QPushButton("remove");
    QBoxLayout* headerLayout = new QBoxLayout( QBoxLayout::LeftToRight );
    headerLayout->addWidget( new QLabel( "Sprite Renderer" ) );
    headerLayout->addWidget( removeButton );
    QWidget* header = new QWidget();
    header->setLayout( headerLayout );

    QBoxLayout* inspectorLayout = new QBoxLayout( QBoxLayout::TopToBottom );
    inspectorLayout->setContentsMargins( 1, 1, 1, 1 );
    inspectorLayout->addWidget( header );
    inspectorLayout->addWidget( table );

    root = new QWidget();
    root->setLayout( inspectorLayout );

    connect( removeButton, SIGNAL(clicked(bool)), mainWindow, SLOT(CommandRemoveSpriteRendererComponent()));
}
