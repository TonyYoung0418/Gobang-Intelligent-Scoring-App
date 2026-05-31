package com.example.gobang;

//描述棋盘上的一个位置
public class Point {
    private int x;
    private int y;
    private int type;
    private int score;
    public Point(int x, int y, int type, int score) {
        this.x = x;
        this.y = y;
        this.type = type;
        this.score = score;
    }

    public Point(int x, int y, int type) {
        this.x = x;
        this.y = y;
        this.type = type;
        this.score = 0;
    }

    public Point(int x, int y) {
        this.x = x;
        this.y = y;
        this.type = -1;
        this.score = 0;
    }

    public int getX() {
        return x;
    }

    public void setX(int x) {
        this.x = x;
    }

    public int getType() {
        return type;
    }

    public void setType(int type) {
        this.type = type;
    }

    public int getY() {
        return y;
    }

    public void setY(int y) {
        this.y = y;
    }

    public int getScore() {
        return score;
    }

    public void setScore(int val) {
        score = val;
    }
}