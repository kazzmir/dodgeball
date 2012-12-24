#include "world.h"
#include "util/graphics/bitmap.h"
#include "util/input/input-manager.h"
#include "util/funcs.h"

#include <math.h>

using std::vector;

namespace Dodgeball{

static const double gravity = 1.1;

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
    
double Field::getFriction() const {
    return 1.0;
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
control(false),
hasBall(false),
facing(FaceRight){
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
    
bool Player::hasControl() const {
    return this->control;
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

    if (hold.left && !hold.right && !hold.up && !hold.down){
        facing = FaceLeft;
    }

    if (hold.left && hold.up && !hold.right && !hold.down){
        facing = FaceUpLeft;
    }
    
    if (hold.left && hold.down && !hold.right && !hold.up){
        facing = FaceDownLeft;
    }
    
    if (hold.right && !hold.left && !hold.up && !hold.down){
        facing = FaceRight;
    }
    
    if (hold.right && hold.up && !hold.left && !hold.down){
        facing = FaceUpRight;
    }
    
    if (hold.right && hold.down && !hold.left && !hold.up){
        facing = FaceDownRight;
    }

    if (hold.up && !hold.down && !hold.left && !hold.right){
        facing = FaceUp;
    }
    
    if (hold.down && !hold.up && !hold.left && !hold.right){
        facing = FaceDown;
    }

    if (handler.action){
        doAction(world);
    }
}

void Player::throwBall(Ball & ball){
    double vx = 0;
    double vy = 0;
    double vz = 0;
    switch (facing){
        case FaceLeft: {
            vx = -10;
            vy = 0;
            vz = 15;
            break;
        }
        case FaceRight: {
            vx = 10;
            vy = 0;
            vz = 15;
            break;
        }
        case FaceUp: {
            vx = 0;
            vy = -10;
            vz = 15;
            break;
        }
        case FaceDown: {
            vx = 0;
            vy = 10;
            vz = 15;
            break;
        }
        case FaceUpLeft: {
            vx = -8;
            vy = -8;
            vz = 15;
            break;
        }
        case FaceUpRight: {
            vx = 8;
            vy = -8;
            vz = 15;
            break;
        }
        case FaceDownLeft: {
            vx = -8;
            vy = 8;
            vz = 15;
            break;
        }
        case FaceDownRight: {
            vx = 8;
            vy = 8;
            vz = 15;
            break;
        }
    }

    ball.doThrow(vx, vy, vz);
}

void Player::doAction(World & world){
    if (hasBall){
        Ball & ball = world.getBall();
        throwBall(ball);
        hasBall = false;
    } else {
        if (Util::distance(getX(), getY(), world.getBall().getX(), world.getBall().getY()) < 20){
            world.getBall().grab(this);
            hasBall = true;
        }
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
    if (hasControl()){
        work.ellipseFill((int) camera.computeX(x), (int) camera.computeY(y), 20, 10, Graphics::makeColor(0, 0, 255));
    }
    if (hasBall){
        work.ellipseFill((int) camera.computeX(x), (int) camera.computeY(y), 20, 10, Graphics::makeColor(255, 255, 0));
        work.ellipseFill((int) camera.computeX(x), (int) camera.computeY(y), 21, 11, Graphics::makeColor(255, 255, 0));
    }
    work.ellipseFill((int) camera.computeX(x), (int) camera.computeY(y - height / 2), 10, height / 2, Graphics::makeColor(255, 0, 0));
    work.circleFill((int) camera.computeX(x + 3), (int) camera.computeY(y - height * 3 / 4), 5, Graphics::makeColor(255, 255, 255));
}

double Player::getX() const {
    return x;
}
    
double Player::getY() const {
    return y;
}

Team::Team(Side side, const Field & field):
side(side){
    switch (side){
        case LeftSide: populateLeft(field); break;
        case RightSide: populateRight(field); break;
    }
}

static Util::ReferenceCount<Player> makePlayer(double x, double y){
    return Util::ReferenceCount<Player>(new Player(x, y));
}

void Team::populateLeft(const Field & field){
    map.set(Keyboard::Key_Q, Cycle);
    double width = field.getWidth() / 2;
    double height = field.getHeight();
    players.push_back(makePlayer(width / 5, height / 2));
    players.push_back(makePlayer(width / 2, height / 4));
    players.push_back(makePlayer(width / 2, height * 3 / 4));
    players.push_back(makePlayer(0, height / 2));
    players.push_back(makePlayer(width / 2, 0));
    players.push_back(makePlayer(width / 2, height));
}

void Team::populateRight(const Field & field){
    double width = field.getWidth() / 2;
    double height = field.getHeight();
    players.push_back(makePlayer(field.getWidth() - width / 5, height / 2));
    players.push_back(makePlayer(field.getWidth() - width / 2, height / 4));
    players.push_back(makePlayer(field.getWidth() - width / 2, height * 3 / 4));
    players.push_back(makePlayer(field.getWidth() - 0, height / 2));
    players.push_back(makePlayer(field.getWidth() - width / 2, 0));
    players.push_back(makePlayer(field.getWidth() - width / 2, height));
}

void Team::enableControl(){
    if (players.size() > 0){
        players[0]->setControl(true);
    }
}

void Team::cycleControl(){
    for (int i = 0; i < players.size(); i++){
        if (players[i]->hasControl()){
            int use = (i + 1) % players.size();
            players[i]->setControl(false);
            players[use]->setControl(true);
            break;
        }
    }
}

void Team::draw(const Graphics::Bitmap & work, const Camera & camera){
    for (vector<Util::ReferenceCount<Player> >::iterator it = players.begin(); it != players.end(); it++){
        const Util::ReferenceCount<Player> & player = *it;
        player->draw(work, camera);
    }
}

void Team::act(World & world){
    class Handler: public InputHandler<Input> {
    public:
        Handler(Team & team):
        team(team){
        }

        Team & team;

        void press(const Input & out, Keyboard::unicode_t unicode){
            switch (out){
                case Cycle: {
                    team.cycleControl();
                    break;
                }
            }
        }

        void release(const Input & out, Keyboard::unicode_t unicode){
        }
    };

    Handler handler(*this);
    InputManager::handleEvents(map, InputSource(0, 0), handler);

    for (vector<Util::ReferenceCount<Player> >::iterator it = players.begin(); it != players.end(); it++){
        const Util::ReferenceCount<Player> & player = *it;
        player->act(world);
    }
}

Ball::Ball(double x, double y):
x(x),
y(y),
z(0),
angle(Util::rnd(360)),
velocityX(0),
velocityY(0),
velocityZ(0),
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
    holder = NULL;
}

void Ball::doThrow(double velocityX, double velocityY, double velocityZ){
    ungrab();
    this->velocityX = velocityX;
    this->velocityY = velocityY;
    this->velocityZ = velocityZ;
}

double Ball::getX() const {
    return x;
}

double Ball::getY() const {
    return y;
}

void Ball::act(const Field & field){
    if (grabbed && holder != NULL){
        this->x = holder->getX();
        this->y = holder->getY();
        /* FIXME: relative to the holder's hands */
        this->z = 15;
    } else {
        x += velocityX;
        y += velocityY;
        z += velocityZ;

        if (z > 0){
            velocityZ -= gravity;
        } else {
            velocityZ = -velocityZ / 2;
            if (velocityZ < gravity){
                velocityZ = 0;
            }
            z = 0;

            if (velocityX > field.getFriction()){
                velocityX -= field.getFriction();
            } else if (velocityX < -field.getFriction()){
                velocityX += field.getFriction();
            } else {
                velocityX = 0;
            }

            if (velocityY > field.getFriction()){
                velocityY -= field.getFriction();
            } else if (velocityY < -field.getFriction()){
                velocityY += field.getFriction();
            } else {
                velocityY = 0;
            }

        }

        angle += fabs(velocityX) + fabs(velocityY);
        if (angle > 360){
            angle -= 360;
        }
        if (angle < 0){
            angle += 360;
        }
    }
}

void Ball::draw(const Graphics::Bitmap & work, const Camera & camera){
    int size = 10;
    int middleX = camera.computeX(x);
    int middleY = camera.computeY(y - z - size);

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
team1(Team::LeftSide, field),
team2(Team::RightSide, field){
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
    ball.act(field);
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
