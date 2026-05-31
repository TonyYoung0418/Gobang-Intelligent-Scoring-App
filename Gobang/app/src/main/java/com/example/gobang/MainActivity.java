package com.example.gobang;

import androidx.annotation.Nullable;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Typeface;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.Window;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends AppCompatActivity {

    private static final int REQUEST_CODE_SERVER = 1;
    private static final int REQUEST_CODE_CONFIG = 2;
    private static final int REQUEST_CODE_SHARE= 3;

    private Graph graph;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        getSupportActionBar().setDisplayOptions(ActionBar.DISPLAY_SHOW_HOME | ActionBar.DISPLAY_SHOW_TITLE);
        getSupportActionBar().setIcon(R.mipmap.icon);
        getSupportActionBar().setTitle("五子棋智能评分系统");
        graph = findViewById(R.id.graph);
        Button newgame = findViewById(R.id.newgame);
        Button goback = findViewById(R.id.goback);
        Button hint = findViewById(R.id.hint);
        
        Typeface typeFace = Typeface.createFromAsset(this.getAssets(), "font2.ttf");
        TextView score = findViewById(R.id.textView);
        score.setTypeface(typeFace);

        ProgressBar bar = findViewById(R.id.progressBar);
        bar.setVisibility(View.INVISIBLE);
        score.setTextSize(30f);
        graph.bindTextView(score);
        graph.bindProgressBar(bar);
        newgame.setOnClickListener(v -> {
            graph.clear();
        });
        goback.setOnClickListener(v -> {
            graph.undo();
        });
        hint.setOnClickListener(v -> {
            graph.hint();
        });

    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) { //此方法用于初始化菜单
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        //菜单的响应事件，根据ItemId辨别响应事件
        int id = item.getItemId();
        switch (id) {
            case R.id.server:
                startActivityForResult(new Intent(this, SetServer.class), REQUEST_CODE_SERVER);
                return true;
            case R.id.config:
                startActivityForResult(new Intent(this, Config.class), REQUEST_CODE_CONFIG);
                return true;
            case R.id.about:
                startActivity(new Intent(this, About.class));
                return true;
//            case R.id.share:
//                if (graph.isWaiting()) {
//                    Toast.makeText(MainActivity.this, "请等待电脑计算完毕后再分享", Toast.LENGTH_LONG).show();
//                }
//                else {
//                    Intent intent = new Intent(this, Share.class);
//                    intent.putExtra(Share.FLAG, graph.dump());
//                    startActivityForResult(intent, REQUEST_CODE_SHARE);
//                    return true;
//                }
            default:
                break;
        }

        return super.onOptionsItemSelected(item);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_CODE_SERVER) {
            String ip = SetServer.getIP(this);
            graph.setIP(ip);
        }
        else if (requestCode == REQUEST_CODE_CONFIG) {
            boolean first = Config.isFirst(this);
            graph.setFirst(first);
            int diff = Config.getDiff(this);
            graph.setDiff(diff);
        }
        else if (requestCode == REQUEST_CODE_SHARE) {
            //Toast.makeText(MainActivity.this, Share.getResult(), Toast.LENGTH_LONG).show();
            graph.load(Share.getResult());
        }
    }
}