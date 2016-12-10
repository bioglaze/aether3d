#include "DirectionalLightInspector.hpp"
#include "GameObject.hpp"
#include "DirectionalLightComponent.hpp"
#include "System.hpp"
#include <QLabel>
#include <QBoxLayout>
#include <QCheckBox>
#include <QColorDialog>
#include <QPushButton>

void DirectionalLightInspector::Init( QWidget* mainWindow )
{
    auto componentName = new QLabel( "Directional Light" );
    shadowCheck = new QCheckBox( "Casts Shadow" );

    removeButton = new QPushButton("remove");
    QBoxLayout* headerLayout = new QBoxLayout( QBoxLayout::LeftToRight );
    headerLayout->addWidget( componentName );
    headerLayout->addWidget( removeButton );
    QWidget* header = new QWidget();
    header->setLayout( headerLayout );

    colorButton = new QPushButton( "color" );

    QWidget* colorWidget = new QWidget();
    QBoxLayout* colorLayout = new QBoxLayout( QBoxLayout::LeftToRight );
    colorLayout->addWidget( colorButton );
    colorWidget->setLayout( colorLayout );

    auto inspectorLayout = new QBoxLayout( QBoxLayout::TopToBottom );
    inspectorLayout->setContentsMargins( 1, 1, 1, 1 );
    inspectorLayout->addWidget( header );
    inspectorLayout->addWidget( colorWidget );
    inspectorLayout->addWidget( shadowCheck );

    root = new QWidget();
    root->setLayout( inspectorLayout );

    colorDialog = new QColorDialog( root );

    connect( shadowCheck, SIGNAL(stateChanged(int)), this, SLOT(ShadowStateChanged(int)) );
    connect( mainWindow, SIGNAL(GameObjectSelected(std::list< ae3d::GameObject* >)),
             this, SLOT(GameObjectSelected(std::list< ae3d::GameObject* >)) );
    connect( removeButton, SIGNAL(clicked(bool)), mainWindow, SLOT(CommandRemoveDirectionalLightComponent()));

    connect( colorButton, &QPushButton::clicked, [=]{ colorDialog->show(); } );
    connect( colorDialog, &QColorDialog::colorSelected,
        [=]( const QColor& color )
        {
            colorDialog->hide();
            gameObject->GetComponent< ae3d::DirectionalLightComponent >()->SetColor( ae3d::Vec3( color.redF(), color.greenF(), color.blueF() ) );
        });

    connect( colorDialog, &QColorDialog::currentColorChanged,
        [=]( const QColor& color )
        {
            gameObject->GetComponent< ae3d::DirectionalLightComponent >()->SetColor( ae3d::Vec3( color.redF(), color.greenF(), color.blueF() ) );
        });
}

void DirectionalLightInspector::ShadowStateChanged( int enabled )
{
    ae3d::System::Assert( gameObject, "Needs game object" );
    ae3d::System::Assert( gameObject->GetComponent< ae3d::DirectionalLightComponent >() != nullptr, "Needs dir light" );

    gameObject->GetComponent< ae3d::DirectionalLightComponent >()->SetCastShadow( enabled == 2 ? true : false, 1024 );
}

void DirectionalLightInspector::GameObjectSelected( std::list< ae3d::GameObject* > gameObjects )
{
    if (gameObjects.empty())
    {
        gameObject = nullptr;
    }
    else
    {
        gameObject = gameObjects.front();
        auto dirLight = gameObject->GetComponent< ae3d::DirectionalLightComponent >();

        if (dirLight != nullptr)
        {
            shadowCheck->setChecked( dirLight->CastsShadow() );
        }
    }
}
