#include "RendererGame.hpp"
#include <iostream>

RendererGame::RendererGame() :
    m_board(nullptr), m_gameOver(false)
{

}

void RendererGame::initializeGL(){
    //
}

void RendererGame::paintGL(){
    if(m_gameOver){
        drawGameOverScreen();
        return;
    }
    QPainter painter(this);
    painter.setPen(Qt::black);
    painter.fillRect(0,0,this->width(), this->height(),QBrush(Qt::black));
    if(m_board){
        int cellSize = this->height() / m_board->getHeight();
        int marginLeft = (this->width() - m_board->getWidth() * cellSize) / 2;
        for(int i = 0; i < m_board->getHeight(); ++i){
            for(int j = 0; j < m_board->getWidth(); ++j){
                if(m_board->getCell(j,i) != EMPTY_CELL){
                    drawBlock(painter, marginLeft + j * cellSize, i * cellSize, cellSize, m_board->getCharColor(m_board->getCell(j,i)));
                    //painter.fillRect(marginLeft + j * cellSize, i * cellSize, cellSize, cellSize, QBrush(m_board->getCharColor(m_board->getCell(j,i))));
                }else{
                    painter.setPen(Qt::blue);
                    painter.drawRect(marginLeft + j * cellSize, i * cellSize, cellSize, cellSize);
                }
            }
        }
    }
}
