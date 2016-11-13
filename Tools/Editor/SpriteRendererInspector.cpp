#include "SpriteRendererInspector.hpp"
#include <iomanip>
#include <QLabel>
#include <QBoxLayout>
#include <QWidget>
#include <QString>
#include <QTableWidgetItem>
#include <QPushButton>
#include <QString>
#include "GameObject.hpp"
#include "SpriteRendererComponent.hpp"

QString FloatToQString( float f );

void SpriteRendererInspector::Init( QWidget* mainWindow )
{
    table = new QTableWidget( 1, 5 );
    table->setItem( 0, 0, new QTableWidgetItem() );
    table->setItem( 0, 1, new QTableWidgetItem() );
    table->setItem( 0, 2, new QTableWidgetItem() );
    table->setItem( 0, 3, new QTableWidgetItem() );
    table->setItem( 0, 4, new QTableWidgetItem() );
    table->setHorizontalHeaderLabels( QString("path;x;y;x scale;y scale").split(";") );
    table->setVerticalHeaderLabels( QString("Sprite").split(";") );

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
    connect( mainWindow, SIGNAL(GameObjectSelected(std::list< ae3d::GameObject* >)),
             this, SLOT(GameObjectSelected(std::list< ae3d::GameObject* >)) );
}

void SpriteRendererInspector::ApplyFieldsIntoSelectedSpriteRenderer()
{
    std::string spritePath( table->item( 0, 0 )->text().toUtf8().constData() );
    float x, y, width, height;

    try
    {
        x = std::stof( table->item( 0, 1 )->text().toUtf8().constData() );
    }
    catch (std::invalid_argument&)
    {
        x = 0;
        table->item( 0, 0 )->setText( "0" );
    }

    try
    {
        y = std::stof( table->item( 0, 2 )->text().toUtf8().constData() );
    }
    catch (std::invalid_argument&)
    {
        y = 0;
        table->item( 0, 2 )->setText( "0" );
    }

    try
    {
        width = std::stof( table->item( 0, 3 )->text().toUtf8().constData() );
    }
    catch (std::invalid_argument&)
    {
        width = 0;
        table->item( 0, 3 )->setText( "0" );
    }

    try
    {
        height = std::stof( table->item( 0, 4 )->text().toUtf8().constData() );
    }
    catch (std::invalid_argument&)
    {
        height = 0;
        table->item( 0, 4 )->setText( "0" );
    }

    emit SpriteRendererModified( spritePath, x, y, width, height );
}

void SpriteRendererInspector::ApplySelectedSpriteRendererIntoFields( const ae3d::SpriteRendererComponent& spriteRenderer )
{
    //if (spriteRenderer.GetSpriteCount() > 0)
    {
        table->item( 0, 0 )->setText( spriteRenderer.GetSpriteInfo( 0 ).path.c_str() );
        table->item( 0, 1 )->setText( FloatToQString( spriteRenderer.GetSpriteInfo( 0 ).x ) );
        table->item( 0, 2 )->setText( FloatToQString( spriteRenderer.GetSpriteInfo( 0 ).y ) );
        table->item( 0, 3 )->setText( FloatToQString( spriteRenderer.GetSpriteInfo( 0 ).width ) );
        table->item( 0, 4 )->setText( FloatToQString( spriteRenderer.GetSpriteInfo( 0 ).height ) );
    }
}

void SpriteRendererInspector::GameObjectSelected( std::list< ae3d::GameObject* > gameObjects )
{
    if (gameObjects.empty() || !gameObjects.front()->GetComponent< ae3d::SpriteRendererComponent >())
    {
        return;
    }

    disconnect( table, &QTableWidget::itemChanged, table, nullptr );
    ApplySelectedSpriteRendererIntoFields( *gameObjects.front()->GetComponent< ae3d::SpriteRendererComponent >() );
    connect( table, &QTableWidget::itemChanged, [&](QTableWidgetItem*) { ApplyFieldsIntoSelectedSpriteRenderer(); });
}
