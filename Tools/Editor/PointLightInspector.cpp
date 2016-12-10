#include "PointLightInspector.hpp"
#include "GameObject.hpp"
#include "PointLightComponent.hpp"
#include "System.hpp"
#include <QLabel>
#include <QBoxLayout>
#include <QColorDialog>
#include <QCheckBox>
#include <QIntValidator>
#include <QLineEdit>
#include <QPushButton>

QString FloatToQString( float f );

void PointLightInspector::Init( QWidget* mainWindow )
{
    auto componentName = new QLabel( "Point Light" );
    shadowCheck = new QCheckBox( "Casts Shadow" );

    removeButton = new QPushButton("remove");
    QBoxLayout* headerLayout = new QBoxLayout( QBoxLayout::LeftToRight );
    headerLayout->addWidget( componentName );
    headerLayout->addWidget( removeButton );
    QWidget* header = new QWidget();
    header->setLayout( headerLayout );

    QLabel* radiusTitle = new QLabel( "Radius" );

    radiusInput = new QLineEdit();
    radiusInput->setMaxLength( 3 );
    radiusInput->setMaximumWidth( 40 );
    radiusInput->setValidator( new QIntValidator() );

    QWidget* radiusWidget = new QWidget();
    QBoxLayout* radiusLayout = new QBoxLayout( QBoxLayout::LeftToRight );
    radiusLayout->addWidget( radiusTitle );
    radiusLayout->addWidget( radiusInput );
    radiusWidget->setLayout( radiusLayout );

    colorButton = new QPushButton( "color" );

    QWidget* colorWidget = new QWidget();
    QBoxLayout* colorLayout = new QBoxLayout( QBoxLayout::LeftToRight );
    colorLayout->addWidget( colorButton );
    colorWidget->setLayout( colorLayout );

    auto inspectorLayout = new QBoxLayout( QBoxLayout::TopToBottom );
    inspectorLayout->setContentsMargins( 1, 1, 1, 1 );
    inspectorLayout->addWidget( header );
    inspectorLayout->addWidget( radiusWidget );
    inspectorLayout->addWidget( colorWidget );
    inspectorLayout->addWidget( shadowCheck );

    root = new QWidget();
    root->setLayout( inspectorLayout );

    colorDialog = new QColorDialog( root );

    connect( shadowCheck, SIGNAL(stateChanged(int)), this, SLOT(ShadowStateChanged(int)) );
    connect( mainWindow, SIGNAL(GameObjectSelected(std::list< ae3d::GameObject* >)),
             this, SLOT(GameObjectSelected(std::list< ae3d::GameObject* >)) );
    connect( removeButton, SIGNAL(clicked(bool)), mainWindow, SLOT(CommandRemovePointLightComponent()));

    connect( colorButton, &QPushButton::clicked, [=]{ colorDialog->show(); } );
    connect( colorDialog, &QColorDialog::colorSelected,
        [=]( const QColor& color )
        {
            colorDialog->hide();
            gameObject->GetComponent< ae3d::PointLightComponent >()->SetColor( ae3d::Vec3( color.redF(), color.greenF(), color.blueF() ) );
        });

    connect( colorDialog, &QColorDialog::currentColorChanged,
        [=]( const QColor& color )
        {
            gameObject->GetComponent< ae3d::PointLightComponent >()->SetColor( ae3d::Vec3( color.redF(), color.greenF(), color.blueF() ) );
        });

    connect( radiusInput, &QLineEdit::textChanged,
        [&](const QString& radius)
        {
            try
            {
                float newRadius = std::stof( radius.toStdString().c_str() );
                gameObject->GetComponent< ae3d::PointLightComponent >()->SetRadius( newRadius );
            }
            catch (std::invalid_argument&)
            {
            }
        });

}

void PointLightInspector::ShadowStateChanged( int enabled )
{
    ae3d::System::Assert( gameObject, "Needs game object" );
    ae3d::System::Assert( gameObject->GetComponent< ae3d::PointLightComponent >() != nullptr, "Needs point light" );

    gameObject->GetComponent< ae3d::PointLightComponent >()->SetCastShadow( enabled != 0, 512 );
}

void PointLightInspector::GameObjectSelected( std::list< ae3d::GameObject* > gameObjects )
{
    if (gameObjects.empty())
    {
        gameObject = nullptr;
    }
    else
    {
        gameObject = gameObjects.front();
        auto pointLight = gameObject->GetComponent< ae3d::PointLightComponent >();

        if (pointLight != nullptr)
        {
            shadowCheck->setChecked( pointLight->CastsShadow() );
            radiusInput->setText( FloatToQString( pointLight->GetRadius() ) );
        }
    }
}
