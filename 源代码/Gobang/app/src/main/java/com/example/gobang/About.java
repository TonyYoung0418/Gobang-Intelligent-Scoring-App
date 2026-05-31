package com.example.gobang;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;

import android.graphics.Typeface;
import android.os.Bundle;
import android.widget.Button;
import android.widget.TextView;

//“关于”界面
public class About extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_about);
        getSupportActionBar().setDisplayOptions(ActionBar.DISPLAY_SHOW_HOME | ActionBar.DISPLAY_SHOW_TITLE);
        getSupportActionBar().setIcon(R.mipmap.icon);
        getSupportActionBar().setTitle("五子棋智能评分系统");
        Typeface typeFace = Typeface.createFromAsset(this.getAssets(), "font2.ttf");

        TextView author = findViewById(R.id.author);
        author.setTextSize(30f);
        author.setTypeface(typeFace);

        TextView qq = findViewById(R.id.QQ);
        qq.setTextSize(30f);
        qq.setTypeface(typeFace);

        Button back = findViewById(R.id.button2);
        back.setOnClickListener(v -> {
            finish();
        });
    }
}