# Cheatsheet for Material Expression Nodes

### Adding expression
> Replace UMaterialExpression with the type your trying to add
```c++
else if (Type->Type == "MaterialExpression") {
  UMaterialExpression* Expression = Cast<UMaterialExpression>(Expression);

  Expression = Expression;
}
```

### Adding expression input
> Replace "Input" with the name of the input

```c++
Expression->Input = CreateExpressionInput(Properties, CreatedExpressionMap, "Input");
```

## Expression Property

##### Float Property
```c++
if (float Property; Properties->TryGetNumberField("Property", Property)) 
  Expression->Property = Property;
```

##### Int Property
```c++
if (int Property; Properties->TryGetNumberField("Property", Property)) 
  Expression->Property = Property;
```

##### Bool Property
```c++
if (bool Property; Properties->TryGetBoolField("Property", Property)) 
  Expression->Property = Property;
```

----------------

##### FName Property
```c++
if (FString VarProperty; Properties->TryGetStringField("Property", VarProperty))
  Expression->Property = FName(VarProperty);
```

##### Enum Property
```c++
if (FString VarProperty; Properties->TryGetStringField("Property", VarProperty))
  Expression->Property = FName(VarProperty);
```

##### Guid Property
```c++
if (FString Property; Properties->TryGetStringField("Property", Property))
  Expression->Property = static_cast<Enum>(StaticEnum<Enum>()->GetValueByNameString(Property));
```

##### Linear Color Property
```c++
if (const TSharedPtr<FJsonObject>* Property; Properties->TryGetObjectField("Property", Property))
  Expression->Property = FMathUtilities::ObjectToLinearColor(Property->Get());
```
