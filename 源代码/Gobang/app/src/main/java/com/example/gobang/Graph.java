package com.example.gobang;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.graphics.drawable.BitmapDrawable;
import android.media.MediaPlayer;
import android.os.Looper;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.Nullable;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.IOException;
import java.util.ArrayList;

import okhttp3.FormBody;
import okhttp3.HttpUrl;
import okhttp3.MediaType;
import okhttp3.OkHttpClient;
import okhttp3.Request;
import okhttp3.RequestBody;
import okhttp3.Response;


public class Graph extends View {

    private static final int EMPTY = -1;
    private static final int BLACK = 0; //电脑玩家
    private static final int WHITE = 1; //人
    private static final int LOSE = 0;
    private static final int WIN = 1;
    private static final int TIE = 2;
    private static final int UNKNOWN = 3;
    private static final MediaType JSON = MediaType.get("application/json; charset=utf-8");
    private static final String[] resText = {"败", "胜", "平"};
    private static final int BASE_QR_SIZE = (15 * 15 + 1) / 2;

    private float[] offset = new float[15];
    private ArrayList<Point> pieces = new ArrayList<Point>();
    private int[][] graph = new int[15][15];
    private Paint linePaint = new Paint();
    private Paint resPaint = new Paint();
    private Paint framePaint = new Paint();
    private Paint hintPaint = new Paint();
    private boolean waiting = false;
    private int result = -1;
    private TextView scoreView;
    private int score = 0;
    private Point hintPos = new Point(-1, -1, -1);
    private String ip = SetServer.getIP(getContext());
    private boolean first = Config.isFirst(getContext());
    private boolean firstNow = Config.isFirst(getContext());
    private int diff = Config.getDiff(getContext());
    private ProgressBar progressBar;
    private MediaPlayer mediaPlayer = MediaPlayer.create(getContext(), R.raw.music);
    private Bitmap[] image = new Bitmap[2];
    private Rect rect = new Rect(0, 0, 221, 221);
    private RectF dest = new RectF();

    public Graph(Context context) {
        super(context);
        init(null, 0);
    }

    public Graph(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(attrs, 0);
    }

    public Graph(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init(attrs, defStyle);
    }

    public String dump() {
        ArrayList<Integer> tmp = new ArrayList<>();
        for (int i = 0; i < 15; ++i) {
            for (int j = 0; j < 15; ++j) {
                tmp.add(graph[i][j] + 1);
            }
        }
        tmp.add(0);
        String data = "";
        for (int i = 0; i < 15 * 15; i += 2) {
            int enc = tmp.get(i) + tmp.get(i + 1) * 3;
            data += String.valueOf(enc);
        }
        data += String.valueOf(getResult());
        data += String.valueOf(score);
        return data;
    }

    public void load(String value) {
        if (value == null) return;
        if (value.length() < BASE_QR_SIZE + 2) {
            Toast.makeText(getContext(), "二维码格式不正确（1）", Toast.LENGTH_SHORT).show();
            return;
        }
        ArrayList<Integer> tmp = new ArrayList<>();
        for (int i = 0; i < BASE_QR_SIZE; i++) {
            int enc = value.charAt(i) - '0';
            tmp.add(enc % 3);
            tmp.add(enc / 3);
        }
        for (int i = 0; i < tmp.size(); ++i) {
            if (tmp.get(i) < 0 || tmp.get(i) >= 3) {
                Toast.makeText(getContext(), "二维码格式不正确（2）", Toast.LENGTH_SHORT).show();
                return;
            }
        }
        int ptr = 0, delta = 0;
        for (int i = 0; i < 15; ++i) {
            for (int j = 0; j < 15; ++j) {
                graph[i][j] = tmp.get(ptr) - 1;
                if (graph[i][j] == BLACK)
                    delta += 1;
                else if (graph[i][j] == WHITE)
                    delta -= 1;
                ptr += 1;
            }
        }
        try {
            setResult(Integer.parseInt(value.substring(BASE_QR_SIZE, BASE_QR_SIZE + 1)));
            score = Integer.parseInt(value.substring(BASE_QR_SIZE + 1));
        }
        catch (NumberFormatException e) {
            Toast.makeText(getContext(), "二维码格式不正确（3）", Toast.LENGTH_SHORT).show();
            clear();
            return;
        }
        firstNow = (delta == 0);
        hintPos = new Point(-1, -1, -1);
        pieces.clear();
        invalidate();
    }

    //选择一个大小合适的方形区域
    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);

        int size;
        int width = getMeasuredWidth();
        int height = getMeasuredHeight();
        int widthWithoutPadding = width - getPaddingLeft() - getPaddingRight();
        int heightWithoutPadding = height - getPaddingTop() - getPaddingBottom();

        if (widthWithoutPadding > heightWithoutPadding) {
            size = heightWithoutPadding;
        } else {
            size = widthWithoutPadding;
        }

        setMeasuredDimension(size + getPaddingLeft() + getPaddingRight(), size + getPaddingTop() + getPaddingBottom());
    }

    //计算两个点之间的距离
    private static double dist(float x1, float y1, float x2, float y2) {
        float dx = x1 - x2;
        float dy = y1 - y2;
        return Math.sqrt(dx * dx + dy * dy);
    }

    public synchronized boolean isWaiting() {
        return waiting;
    }

    private synchronized void setWaiting(boolean val) {
        waiting = val;
        if (waiting)
            progressBar.setVisibility(View.VISIBLE);
        else
            progressBar.setVisibility(View.INVISIBLE);
    }

    private synchronized int getResult() {
        return result;
    }

    private synchronized void setResult(int val) {
        result = val;
    }

    public void bindTextView(TextView view) {
        scoreView = view;
    }

    public void bindProgressBar(ProgressBar bar) {
        progressBar = bar;
    }

    public void setIP(String IP) {
        ip = IP;
    }

    public void setFirst(boolean val) {
        first = val;
    }

    public void setDiff(int val) {
        diff = val;
    }

    //显示连接错误信息
    public void connectError(){
        Looper.prepare();
        Toast.makeText(getContext(), "无法连接到服务器", Toast.LENGTH_SHORT).show();
    }

    //清空棋盘，开始新的游戏
    public void clear() {
        firstNow = first;
        if (isWaiting()) {
            Toast.makeText(getContext(), "正在等待电脑玩家走棋", Toast.LENGTH_SHORT).show();
            return;
        }
        hintPos = new Point(-1, -1, -1);
        setResult(UNKNOWN);
        for (int i = 0; i < 15; ++i) {
            for (int j = 0; j < 15; ++j)
                graph[i][j] = EMPTY;
        }
        pieces.clear();
        if (!firstNow) {
            graph[7][7] = BLACK;
            pieces.add(new Point(7, 7, BLACK));
        }
        score = 0;
        invalidate();
    }

    //悔棋操作
    public void undo() {
        if (isWaiting()) {
            Toast.makeText(getContext(), "正在等待电脑玩家走棋", Toast.LENGTH_SHORT).show();
            return;
        }
        if (getResult() != UNKNOWN) {
            Toast.makeText(getContext(), "游戏已结束，不能悔棋", Toast.LENGTH_SHORT).show();
            return;
        }
        //恢复到上一步的状态
        hintPos = new Point(-1, -1, -1);
        if (pieces.size() >= 2) {
            for (int i = 0; i < 2; ++i) {
                Point pt = pieces.get(pieces.size() - 1);
                pieces.remove(pieces.size() - 1);
                graph[pt.getX()][pt.getY()] = EMPTY;
            }
            if (pieces.size() >= 1) {
                Point pt = pieces.get(pieces.size() - 1);
                score = pt.getScore();
            }
            else {
                score = 0;
            }
            invalidate();
        }
    }

    public String getURL() {
        return "http://" + ip + ":8888/server";
    }

    //提示走棋的位置
    public void hint() {
        if (isWaiting()) {
            Toast.makeText(getContext(), "正在等待电脑玩家走棋", Toast.LENGTH_SHORT).show();
            return;
        }
        if (getResult() != UNKNOWN) {
            Toast.makeText(getContext(), "游戏已结束，请开始新的游戏", Toast.LENGTH_SHORT).show();
            return;
        }
        setWaiting(true);
        JSONObject json = new JSONObject();
        try {
            json.put("machine", WHITE);
            json.put("person", BLACK);
            json.put("difficulty", 10);
            JSONArray list = new JSONArray();
            for (int i = 0; i < 15; ++i) {
                for (int j = 0; j < 15; ++j)
                    list.put(graph[i][j]);
            }
            json.put("board", list);
        }
        catch (JSONException e) {
            e.printStackTrace();
        }
        RequestBody body = new FormBody.Builder().add("json", json.toString()).build();
        Request request = new Request.Builder().url(getURL()).post(body).build();
        new Thread(() -> {
            try {
                //向服务器发送请求
                OkHttpClient client = new OkHttpClient();
                Response response = client.newCall(request).execute();
                String data = response.body().string();
                JSONObject reply = new JSONObject(data);
                JSONArray position = reply.getJSONArray("position");
                int x = position.getInt(0), y = position.getInt(1);
                //得到提示位置
                hintPos = new Point(x, y, 0);
                invalidate();
            }
            catch (IOException | JSONException e) {
                //e.printStackTrace();
                connectError();
            }
            setWaiting(false);
        }).start();
    }

    //向服务器发送计算走棋请求
    private void postMessage() {
        JSONObject json = new JSONObject();
        try {
            json.put("machine", BLACK);
            json.put("person", WHITE);
            json.put("difficulty", 6 + diff * 2);
            System.out.println(6 + diff * 2);
            JSONArray list = new JSONArray();
            for (int i = 0; i < 15; ++i) {
                for (int j = 0; j < 15; ++j)
                    list.put(graph[i][j]);
            }
            json.put("board", list);
        }
        catch (JSONException e) {
            e.printStackTrace();
        }
        RequestBody body = new FormBody.Builder().add("json", json.toString()).build();
        Request request = new Request.Builder().url(getURL()).post(body).build();
        setWaiting(true);
        new Thread(() -> {
            try {
                OkHttpClient client = new OkHttpClient();
                Response response = client.newCall(request).execute();
                String data = response.body().string();
                JSONObject reply = new JSONObject(data);
                if (reply.getString("result").equals("lose")) {
                    setResult(LOSE);
                }
                else if (reply.getString("result").equals("win")) {
                    setResult(WIN);
                }
                else if (reply.getString("result").equals("tie")) {
                    setResult(TIE);
                }
                score = Integer.parseInt(reply.getString("score"));
                JSONArray position = reply.getJSONArray("position");
                int x = position.getInt(0), y = position.getInt(1);
                graph[x][y] = BLACK;
                pieces.add(new Point(x, y, BLACK, score));
                mediaPlayer.start();
            }
            catch (IOException e) {
                connectError();
                Point pt = pieces.get(pieces.size() - 1);
                pieces.remove(pieces.size() - 1);
                graph[pt.getX()][pt.getY()] = EMPTY;
            }
            catch (JSONException e) {
                //获胜时没有position
            }
            invalidate();
            setWaiting(false);
        }).start();
    }

    //玩家将棋子放在(x, y)的位置
    private void addPiece(float x, float y) {
        if (getResult() != UNKNOWN) {
            clear();
            setResult(UNKNOWN);
            return;
        }
        if (isWaiting()) {
            Toast.makeText(getContext(), "正在等待电脑玩家走棋", Toast.LENGTH_SHORT).show();
            return;
        }
        double mn = 1e9;
        Point pt = new Point(0, 0, WHITE);
        for (int i = 0; i < 15; ++i) {
            for (int j = 0; j < 15; ++j) {
                double distance = dist(x, y, offset[i], offset[j]);
                if (distance < mn) {
                    mn = distance;
                    pt.setX(j);
                    pt.setY(i);
                }
            }
        }
        if (graph[pt.getX()][pt.getY()] == EMPTY) {
            hintPos = new Point(-1, -1, -1);
            pieces.add(pt);
            graph[pt.getX()][pt.getY()] = WHITE;
            mediaPlayer.start();
            invalidate();
            postMessage();
        }
    }

    //初始化Graph类
    private void init(AttributeSet attrs, int defStyle) {
        hintPaint.setColor(Color.argb(200, 240, 240, 30));
        hintPaint.setAntiAlias(true);
        hintPaint.setStrokeWidth(2f);

        framePaint.setStrokeWidth(2.5f);
        framePaint.setColor(Color.RED);
        framePaint.setStyle(Paint.Style.STROKE);
        framePaint.setAntiAlias(true);

        resPaint = new Paint();
        resPaint.setColor(Color.argb(160, 255, 50, 60));
        resPaint.setAntiAlias(true);
        resPaint.setTextSize(1000f);
        resPaint.setTextAlign(Paint.Align.CENTER);
        Typeface typeFace = Typeface.createFromAsset(getContext().getAssets(), "font.ttf");
        resPaint.setTypeface(typeFace);

        linePaint.setColor(Color.BLACK);
        linePaint.setAntiAlias(true);
        linePaint.setStrokeWidth(3f);

        image[0] = BitmapFactory.decodeStream(this.getResources().openRawResource(R.raw.black));
        image[1] = BitmapFactory.decodeStream(this.getResources().openRawResource(R.raw.white));

        clear();

        setOnTouchListener((v, event) -> {
            float x, y;
            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    x = event.getX();
                    y = event.getY();
                    addPiece(x, y);
            }
            return false;
        });
    }

    //在屏幕上绘制棋盘和其他的提示信息
    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        final int width = getHeight() > getWidth() ? getWidth() : getHeight();
        final float margin = width / 20.0f;
        final float step = (width - margin * 2) / 14f;
        final float radius = step / 3;
        //绘制棋盘的线条
        for (int i = 0; i < 15; ++i) {
            float pos = i * step;
            offset[i] = pos + margin;
            canvas.drawLine(margin, pos + margin,
                    width - margin, pos + margin, linePaint);
            canvas.drawLine(pos + margin, margin, pos + margin, width - margin, linePaint);
        }
        //绘制棋子
        for (int i = 0; i < 15; ++i) {
            for (int j = 0; j < 15; ++j) {
                if (graph[i][j] != EMPTY) {
                    dest.left = offset[j] - radius;
                    dest.right = offset[j] + radius;
                    dest.top = offset[i] - radius;
                    dest.bottom = offset[i] + radius;
                    if (firstNow)
                        canvas.drawBitmap(image[graph[i][j]^1], rect, dest, linePaint);
                    else
                        canvas.drawBitmap(image[graph[i][j]], rect, dest, linePaint);
                    //canvas.drawCircle(offset[j], offset[i], radius, paint[graph[i][j]]);
                }
            }
        }

        for (int i = 1; i <= 2; ++i) {
            if (pieces.size() >= i) {
                Point pt = pieces.get(pieces.size() - i);
                if (pt.getType() == BLACK) {
                    framePaint.setColor(score == -100 ? Color.RED : Color.GREEN);
                    canvas.drawCircle(offset[pt.getY()], offset[pt.getX()], radius, framePaint);
                }
            }
        }

        //绘制提示信息
        if (hintPos.getX() >= 0 && hintPos.getY() >= 0) {
            float x = offset[hintPos.getY()], y = offset[hintPos.getX()];
            float range = radius * 1.1f;
            Path path = new Path();
            path.moveTo(x, y - range);
            path.lineTo(x - range, y);
            path.lineTo(x, y + range);
            path.lineTo(x + range, y);
            canvas.drawPath(path, hintPaint);
        }

        if (getResult() != UNKNOWN) {
            int r = getResult();
            canvas.drawText(resText[r], (float) width / 1.9f, (float) width / 1.3f, resPaint);
        }
        scoreView.setText("棋局评分：" + score);
    }

}