#include "world.h"
#include "util/graphics/bitmap.h"
#include "util/input/input-manager.h"
#include "util/funcs.h"

#include <math.h>

using std::vector;

namespace Dodgeball{

Camera::Camera():
x(0),
y(0),
zoom(1){
}

void Camera::zoomIn(double amount){
    zoom += amount;
}

void Camera::zoomOut(double amount){
    zoom -= amount;
    if (zoom < 0.1){
        zoom = 0.1;
    }
}

void Camera::normalZoom(){
    zoom = 1;
}

int Camera::getWidth() const {
    return 2 * width / getZoom();
}

int Camera::getHeight() const {
    return 2 * height / getZoom();
}

double Camera::getX1() const {
    double x1 = getX() - width / getZoom();
    /*
       if (x1 < 0){
       return 0;
       }
       */
    return x1;
}
    
double Camera::getX2() const {
    double x2 = getX() + width / getZoom();
    /*
       if (x2 > 1000){
       return 1000 - getWidth();
       }
       */
    return x2;
}
    
double Camera::getY1() const {
    double y1 = getY() - height / getZoom();
    /*
       if (y1 < 0){
       return 0;
       }
       */
    return y1;
}
    
double Camera::getY2() const {
    double y2 = getY() + height / getZoom();
    /*
       if (y2 > 1000){
       return y2 - getHeight();
       }
       */
    return y2;
}

double Camera::computeX(double x) const {
    return x - getX1();
}

double Camera::computeY(double y) const {
    return y - getY1();
}

void Camera::moveTowards(double x, double y){
    this->x = (x + this->x) / 2;
    this->y = (y + this->y) / 2;
}

void Camera::moveTo(double x, double y){
    this->x = x;
    this->y = y;
}

void Camera::moveRight(int much){
    this->x += much;
}
    
void Camera::moveLeft(int much){
    this->x -= much;
}
    
void Camera::moveUp(int much){
    this->y -= much;
}

void Camera::moveDown(int much){
    this->y += much;
}

double Camera::getX() const {
    return x;
}

double Camera::getY() const {
    return y;
}

double Camera::getZoom() const {
    return zoom;
}

Field::Field(int width, int height):
width(width),
height(height){
}

int Field::getWidth() const {
    return width;
}

int Field::getHeight() const {
    return height;
}

void Field::draw(const Graphics::Bitmap & work, const Camera & camera){
    int margin = 30;

    /* the outside edge of the line is the boundary */
    int x1 = 0 + margin;
    int x2 = width - margin;
    int y1 = 0 + margin;
    int y2 = height - margin;

    int middle = (x1 + x2) / 2;

    work.fill(Graphics::makeColor(0, 255, 0));

    Graphics::Color white = Graphics::makeColor(255, 255, 255);

    /* center line */
    work.rectangleFill(camera.computeX(middle - 5), camera.computeY(y1),
                       camera.computeX(middle + 5), camera.computeY(y2),
                       white);

    /* left side */
    work.rectangleFill(camera.computeX(x1), camera.computeY(y1),
                       camera.computeX(x1 + 10), camera.computeY(y2),
                       white);

    /* right side */
    work.rectangleFill(camera.computeX(x2), camera.computeY(y1),
                       camera.computeX(x2 - 10), camera.computeY(y2),
                       white);

    /* top side */
    work.rectangleFill(camera.computeX(x1), camera.computeY(y1),
                       camera.computeX(x2), camera.computeY(y1 + 10),
                       white);

    /* bottom side */
    work.rectangleFill(camera.computeX(x1), camera.computeY(y2),
                       camera.computeX(x2), camera.computeY(y2 - 10),
                       white);
}

Player::Player(double x, double y):
x(x),
y(y),
control(false){
    map.set(Keyboard::Key_LEFT, Left);
    map.set(Keyboard::Key_RIGHT, Right);
    map.set(Keyboard::Key_UP, Up);
    map.set(Keyboard::Key_DOWN, Down);
    map.set(Keyboard::Key_A, Action);
}

void Player::act(World & world){
    if (control){
        doInput(world);
    }
}

void Player::setControl(bool what){
    this->control = what;
}

void Player::doInput(World & world){
    class Handler: public InputHandler<Input> {
    public:
        Handler(Player & player):
        player(player),
        action(false){
        }

        Player & player;
        bool action;

        void handle(const Input & out, bool set){
            switch (out){
                case Left: {
                    player.hold.left = set;
                    break;
                }
                case Right: {
                    player.hold.right = set;
                    break;
                }
                case Up: {
                    player.hold.up = set;
                    break;
                }
                case Down: {
                    player.hold.down = set;
                    break;
                }
                default: {
                    break;
                }
            }
        }

        void press(const Input & out, Keyboard::unicode_t unicode){
            handle(out, true);

            switch (out){
                case Action: {
                    action = true;
                    break;
                }
                default: {
                    break;
                }
            }

        }

        void release(const Input & out, Keyboard::unicode_t unicode){
            handle(out, false);
        }
    };

    Handler handler(*this);
    InputManager::handleEvents(map, InputSource(0, 0), handler);

    double speed = 3;

    if (hold.left){
        moveLeft(speed);
    }

    if (hold.right){
        moveRight(speed);
    }

    if (hold.up){
        moveUp(speed);
    }

    if (hold.down){
        moveDown(speed);
    }

    if (handler.action){
        doAction(world);
    }
}

void Player::doAction(World & world){
    if (Util::distance(getX(), getY(), world.getBall().getX(), world.getBall().getY()) < 20){
        world.getBall().grab(this);
    }
}

void Player::moveLeft(double speed){
    this->x -= speed;
}

void Player::moveRight(double speed){
    this->x += speed;
}

void Player::moveUp(double speed){
    this->y -= speed;
}

void Player::moveDown(double speed){
    this->y += speed;
}

void Player::draw(const Graphics::Bitmap & work, const Camera & camera){
    int height = 60;
    work.ellipseFill((int) camera.computeX(x), (int) camera.computeY(y), 10, 5, Graphics::makeColor(32, 32, 32));
    work.ellipseFill((int) camera.computeX(x), (int) camera.computeY(y - height / 2), 10, height / 2, Graphics::makeColor(255, 0, 0));
    work.circleFill((int) camera.computeX(x + 3), (int) camera.computeY(y - height * 3 / 4), 5, Graphics::makeColor(255, 255, 255));
}

double Player::getX() const {
    return x;
}
    
double Player::getY() const {
    return y;
}

Team::Team(const Field & field){
    players.push_back(Util::ReferenceCount<Player>(new Player(Util::rnd(field.getWidth()), Util::rnd(field.getHeight()))));
}

void Team::enableControl(){
    if (players.size() > 0){
        players[0]->setControl(true);
    }
}

void Team::draw(const Graphics::Bitmap & work, const Camera & camera){
    for (vector<Util::ReferenceCount<Player> >::iterator it = players.begin(); it != players.end(); it++){
        const Util::ReferenceCount<Player> & player = *it;
        player->draw(work, camera);
    }
}

void Team::act(World & world){
    for (vector<Util::ReferenceCount<Player> >::iterator it = players.begin(); it != players.end(); it++){
        const Util::ReferenceCount<Player> & player = *it;
        player->act(world);
    }
}

Ball::Ball(double x, double y):
x(x),
y(y),
angle(Util::rnd(360)),
grabbed(false),
moving(false),
holder(NULL){
}

void Ball::grab(Player * holder){
    grabbed = true;
    moving = false;
    this->holder = holder;
}
    
void Ball::ungrab(){
    grabbed = false;
}

double Ball::getX() const {
    return x;
}

double Ball::getY() const {
    return y;
}

void Ball::act(){
    if (grabbed && holder != NULL){
        this->x = holder->getX();
        this->y = holder->getY();
    }

    if (moving){
        angle += 1;
        if (angle > 360){
            angle -= 360;
        }
    }
}

void Ball::draw(const Graphics::Bitmap & work, const Camera & camera){
    int size = 10;
    int middleX = camera.computeX(x);
    int middleY = camera.computeY(y - size);

    work.ellipseFill(camera.computeX(x + size / 2), camera.computeY(y),
                     size, size / 2, Graphics::makeColor(32, 32, 32));

    work.circleFill(middleX, middleY, size,
                    Graphics::makeColor(255, 255, 255));

    double radians = Util::radians(angle);
    work.line(middleX - cos(radians) * size, middleY - sin(radians) * size,
              middleX + cos(radians) * size, middleY + sin(radians) * size,
              Graphics::makeColor(0, 0, 0));

    radians = Util::radians(angle + 90);
    work.line(middleX - cos(radians) * size, middleY - sin(radians) * size,
              middleX + cos(radians) * size, middleY + sin(radians) * size,
              Graphics::makeColor(0, 0, 0));
}

World::World():
field(1200, 600),
ball(400, 300),
team1(field),
team2(field){
    camera.moveTo(field.getWidth() / 2, field.getHeight() / 2);
    map.set(Keyboard::Key_LEFT, Left);
    map.set(Keyboard::Key_RIGHT, Right);
    map.set(Keyboard::Key_UP, Up);
    map.set(Keyboard::Key_DOWN, Down);
    map.set(Keyboard::Key_EQUALS, ZoomIn);
    map.set(Keyboard::Key_MINUS, ZoomOut);

    team1.enableControl();
}

void World::run(){
    class Handler: public InputHandler<Input> {
    public:
        Handler(World & world):
        world(world){
        }

        void press(const Input & out, Keyboard::unicode_t unicode){
            switch (out){
                /*
                case Left: {
                    world.moveLeft();
                    break;
                }
                case Right: {
                    world.moveRight();
                    break;
                }
                case Up: {
                    world.moveUp();
                    break;
                }
                case Down: {
                    world.moveDown();
                    break;
                }
                */
                case ZoomIn: {
                    world.camera.zoomIn(0.02);
                    break;
                }
                case ZoomOut: {
                    world.camera.zoomOut(0.02);
                    break;
                }
            }
        }

        void release(const Input & out, Keyboard::unicode_t unicode){
        }

        World & world;
    };

    Handler handler(*this);

    InputManager::handleEvents(map, InputSource(0, 0), handler);
    ball.act();
    team1.act(*this);
    team2.act(*this);

    camera.moveTowards(ball.getX(), ball.getY());
}

void World::moveLeft(){
    camera.moveLeft(5);
}

void World::moveRight(){
    camera.moveRight(5);
}
    
void World::moveUp(){
    camera.moveUp(5);
}
    
void World::moveDown(){
    camera.moveDown(5);
}

void World::drawPlayers(const Graphics::Bitmap & work){
    team1.draw(work, camera);
    team2.draw(work, camera);
}

void World::draw(const Graphics::Bitmap & screen){
    Graphics::StretchedBitmap work(camera.getWidth(), camera.getHeight(), screen);
    work.start();

    field.draw(work, camera);
    drawPlayers(work);
    ball.draw(work, camera);

    work.finish();
}
    
Ball & World::getBall(){
    return ball;
}

}
